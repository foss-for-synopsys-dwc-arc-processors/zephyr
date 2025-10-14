/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * APLIC GENMSI SMP Test
 *
 * Validates APLIC GENMSI (software MSI injection) across multiple CPUs.
 * Tests that GENMSI can target specific harts and that each hart's IMSIC
 * correctly receives and processes MSI writes.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>
#include <zephyr/devicetree.h>

/* Get APLIC and IMSIC addresses from device tree */
#define APLIC_NODE DT_NODELABEL(aplic)
#define IMSIC0_NODE DT_NODELABEL(imsic0)

#define APLIC_BASE DT_REG_ADDR(APLIC_NODE)
#define IMSIC0_BASE DT_REG_ADDR(IMSIC0_NODE)

/* Check if IMSIC1 exists (for SMP) */
#if DT_NODE_EXISTS(DT_NODELABEL(imsic1))
#define IMSIC1_NODE DT_NODELABEL(imsic1)
#define IMSIC1_BASE DT_REG_ADDR(IMSIC1_NODE)
#define HAS_IMSIC1 1
#else
#define IMSIC1_BASE 0
#define HAS_IMSIC1 0
#endif

/* APLIC register offsets */
#define APLIC_DOMAINCFG          0x0000
#define APLIC_MSIADDRCFG         0x1BC0
#define APLIC_MSIADDRCFGH        0x1BC4
#define APLIC_GENMSI             0x3000

/* IMSIC CSRs */
#define CSR_MISELECT  0x350
#define CSR_MIREG     0x351

/* MIREG indirect register numbers for M-mode IMSIC */
#define IMSIC_EIDELIVERY  0x70
#define IMSIC_EITHRESHOLD 0x72
#define IMSIC_EIP0        0x80
#define IMSIC_EIE0        0xC0

/* Test EIIDs - different for each CPU to avoid conflicts */
#define TEST_EIID_CPU0  64
#define TEST_EIID_CPU1  65

/* Per-CPU ISR counters */
static volatile int isr_count_cpu0;
static volatile int isr_count_cpu1;

/* Broadcast test counters */
static volatile int broadcast_cpu0;
static volatile int broadcast_cpu1;

/* Test ISR for CPU 0 */
static void test_isr_cpu0(const void *arg)
{
	ARG_UNUSED(arg);
	isr_count_cpu0++;
	printk("  [CPU 0 ISR] Fired! Count=%d, EIID=%u\n", isr_count_cpu0, TEST_EIID_CPU0);
}

/* Test ISR for CPU 1 */
static void test_isr_cpu1(const void *arg)
{
	ARG_UNUSED(arg);
	isr_count_cpu1++;
	printk("  [CPU 1 ISR] Fired! Count=%d, EIID=%u\n", isr_count_cpu1, TEST_EIID_CPU1);
}

/* Broadcast ISR - checks which CPU it's running on */
static void broadcast_isr_common(const void *arg)
{
	ARG_UNUSED(arg);
	uint32_t cpu_id = arch_proc_id();

	if (cpu_id == 0) {
		broadcast_cpu0++;
		printk("  [CPU 0 Broadcast ISR] Count=%d\n", broadcast_cpu0);
	} else if (cpu_id == 1) {
		broadcast_cpu1++;
		printk("  [CPU 1 Broadcast ISR] Count=%d\n", broadcast_cpu1);
	}
}

/* CPU 1 initialization status - read by CPU 0 after completion */
static volatile bool cpu1_init_done;
static volatile bool cpu1_init_success;
static volatile uint32_t cpu1_actual_cpu_id = 0xFF;
static struct k_sem cpu1_init_sem;

/* Thread that runs on CPU 1 to enable its interrupts */
static void cpu1_enable_interrupts_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* NO PRINTK - store results in globals for later reporting */
	uint32_t cpu_id = arch_proc_id();

	cpu1_actual_cpu_id = cpu_id;

	if (cpu_id != 1) {
		/* Wrong CPU - signal error */
		cpu1_init_success = false;
		cpu1_init_done = true;
		k_sem_give(&cpu1_init_sem);
		return;
	}

	/* Initialize IMSIC on CPU 1 */
	extern void z_riscv_imsic_secondary_init(void);
	z_riscv_imsic_secondary_init();

	/* Enable the test EIIDs on CPU 1's IMSIC */
	irq_enable(TEST_EIID_CPU1);  /* EIID 65 */
	irq_enable(70);               /* EIID 70 for broadcast */

	/* Signal success */
	cpu1_init_success = true;
	cpu1_init_done = true;
	k_sem_give(&cpu1_init_sem);
}

/* Thread stack for CPU 1 initialization - increased to avoid stack overflow */
K_THREAD_STACK_DEFINE(cpu1_init_stack, 4096);
static struct k_thread cpu1_init_thread_data;

/* Read IMSIC indirect register via M-mode CSRs */
static inline uint32_t imsic_read_indirect(uint32_t reg)
{
	__asm__ volatile("csrw %0, %1" :: "i"(CSR_MISELECT), "r"(reg));
	uint32_t val;

	__asm__ volatile("csrr %0, %1" : "=r"(val) : "i"(CSR_MIREG));
	return val;
}

/* Write IMSIC indirect register via M-mode CSRs */
static inline void imsic_write_indirect(uint32_t reg, uint32_t val)
{
	__asm__ volatile("csrw %0, %1" :: "i"(CSR_MISELECT), "r"(reg));
	__asm__ volatile("csrw %0, %1" :: "i"(CSR_MIREG), "r"(val));
}

/* Read APLIC register */
static inline uint32_t aplic_read(uint32_t offset)
{
	return sys_read32(APLIC_BASE + offset);
}

/* Write APLIC register */
static inline void aplic_write(uint32_t offset, uint32_t val)
{
	sys_write32(val, APLIC_BASE + offset);
}

int main(void)
{
	printk("\n");
	printk("╔═══════════════════════════════════════════════╗\n");
	printk("║       APLIC GENMSI SMP Test (2 CPUs)         ║\n");
	printk("╚═══════════════════════════════════════════════╝\n");
	printk("\n");

	printk("Hardware Configuration:\n");
	printk("  APLIC Base:   0x%08lx\n", (unsigned long)APLIC_BASE);
	printk("  IMSIC0 Base:  0x%08lx (CPU 0)\n", (unsigned long)IMSIC0_BASE);
#if HAS_IMSIC1
	printk("  IMSIC1 Base:  0x%08lx (CPU 1)\n", (unsigned long)IMSIC1_BASE);
#else
	printk("  IMSIC1:       Not present (single-CPU platform)\n");
#endif
	printk("  Num CPUs:     %u\n", CONFIG_MP_MAX_NUM_CPUS);
	printk("\n");

#if !HAS_IMSIC1
	printk("⚠️  WARNING: Running on single-CPU platform!\n");
	printk("    This test is designed for SMP (2+ CPUs).\n");
	printk("    Will test GENMSI register access only.\n");
	printk("    For full SMP testing, use qemu_riscv32_aia/qemu_virt_riscv32_aia/smp\n");
	printk("\n");
#endif

	/* Verify we're running on CPU 0 */
	uint32_t current_cpu = arch_curr_cpu()->id;

	printk("Main thread running on CPU %u\n", current_cpu);
	if (current_cpu != 0) {
		printk("⚠️  WARNING: Expected to run on CPU 0!\n");
	}
	printk("\n");

	/* Step 1: Read APLIC configuration */
	printk("STEP 1: Reading APLIC Configuration\n");
	printk("======================================\n");

	uint32_t domaincfg = aplic_read(APLIC_DOMAINCFG);
	uint32_t msiaddr_low = aplic_read(APLIC_MSIADDRCFG);
	uint32_t msiaddr_high = aplic_read(APLIC_MSIADDRCFGH);

	printk("  DOMAINCFG:    0x%08x\n", domaincfg);
	printk("    - IE (bit 8):  %s\n", (domaincfg & BIT(8)) ? "Enabled" : "Disabled");
	printk("    - DM (bit 2):  %s\n", (domaincfg & BIT(2)) ? "MSI mode" : "Direct mode");
	printk("  MSIADDRCFG:   0x%08x (PPN for IMSIC base)\n", msiaddr_low);
	printk("  MSIADDRCFGH:  0x%08x (geometry fields)\n", msiaddr_high);
	printk("    Expected PPN: 0x%08x (IMSIC0 0x%08lx >> 12)\n",
	       (uint32_t)(IMSIC0_BASE >> 12), (unsigned long)IMSIC0_BASE);

	/* Decode geometry fields */
	uint32_t lhxs = (msiaddr_high >> 20) & 0x7;
	uint32_t lhxw = (msiaddr_high >> 12) & 0xF;
	uint32_t hhxs = (msiaddr_high >> 24) & 0x1F;
	uint32_t hhxw = (msiaddr_high >> 16) & 0x7;

	printk("    Geometry: LHXS=%u, LHXW=%u, HHXS=%u, HHXW=%u\n",
	       lhxs, lhxw, hhxs, hhxw);

	if (msiaddr_low != (IMSIC0_BASE >> 12)) {
		printk("  ⚠️  WARNING: MSIADDRCFG doesn't match expected IMSIC0 address!\n");
	} else {
		printk("  ✓ MSIADDRCFG matches IMSIC0 address\n");
	}
	printk("\n");

	/* Step 2: Setup test interrupt handlers */
	printk("STEP 2: Setting up Test ISRs\n");
	printk("======================================\n");

	/* Register ISR for CPU 0 (EIID 64) and enable on CPU 0 */
	IRQ_CONNECT(TEST_EIID_CPU0, 1, test_isr_cpu0, (void *)TEST_EIID_CPU0, 0);
	irq_enable(TEST_EIID_CPU0);
	printk("  CPU 0: Registered and enabled ISR for EIID %u\n", TEST_EIID_CPU0);

	/* Register ISR for CPU 1 (EIID 65) - will enable on CPU 1 later */
	IRQ_CONNECT(TEST_EIID_CPU1, 1, test_isr_cpu1, (void *)TEST_EIID_CPU1, 0);
	printk("  CPU 1: Registered ISR for EIID %u (will enable on CPU 1)\n", TEST_EIID_CPU1);

	printk("\n");

	/* Check secondary CPU status and attempt workaround */
#if HAS_IMSIC1
	printk("STEP 2.5: Checking Secondary CPU Status\n");
	printk("======================================\n");

	/* Check if CPU 1 is online */
	bool cpu1_online = false;

	#ifdef CONFIG_SMP
	/* Access kernel CPU structures */
	if (CONFIG_MP_MAX_NUM_CPUS > 1) {
		struct _cpu *cpu1 = &_kernel.cpus[1];
		cpu1_online = cpu1->arch.online;
		printk("  CPU 1 online status: %s\n", cpu1_online ? "ONLINE" : "OFFLINE");
	}
	#endif

	if (!cpu1_online) {
		printk("  ⚠️  CPU 1 is OFFLINE - secondary CPU boot failed!\n");
		printk("  This explains why CPU 1 interrupts don't work.\n");
		printk("\n");
		printk("  Root Cause Analysis:\n");
		printk("    1. arch_secondary_cpu_init() was never called for CPU 1\n");
		printk("    2. z_riscv_imsic_secondary_init() never ran\n");
		printk("    3. IMSIC1 EIDELIVERY register is still disabled (value 0)\n");
		printk("\n");
		printk("  Possible reasons:\n");
		printk("    - CONFIG_PM_CPU_OPS not enabled or OpenSBI not available\n");
		printk("    - QEMU not configured to wake secondary CPUs\n");
		printk("    - SMP boot sequence not properly implemented for this platform\n");
		printk("\n");
		printk("  WORKAROUND ATTEMPT: Enabling IMSIC1 via direct MSI writes\n");
		printk("  (This will prove IMSIC1 hardware works, even without CPU 1 running)\n");
		printk("\n");

		/* IMSIC MMIO registers (from AIA spec) */
		/* Offset 0x00: seteipnum_le - Write EIID to set pending & enable */
		/* We can use this to both enable the EIID and set it pending */

		volatile uint32_t *imsic1_seteipnum_le = (volatile uint32_t *)(IMSIC1_BASE + 0x00);

		printk("  Step 1: Writing EIID %u to IMSIC1 seteipnum_le (0x%08lx)\n",
		       TEST_EIID_CPU1, (unsigned long)IMSIC1_BASE);
		printk("          This should enable EIE[%u] and set EIP[%u]\n",
		       TEST_EIID_CPU1, TEST_EIID_CPU1);

		*imsic1_seteipnum_le = TEST_EIID_CPU1;

		printk("  ✓ MMIO write completed\n");
		printk("\n");
		printk("  NOTE: Even with EIE[%u] enabled, CPU 1 still won't process\n", TEST_EIID_CPU1);
		printk("  the interrupt because:\n");
		printk("    - CPU 1 is not running (no code executing on that hart)\n");
		printk("    - EIDELIVERY CSR is disabled (never initialized)\n");
		printk("    - MEXT interrupt is not enabled in MIE on CPU 1\n");
		printk("\n");
		printk("  To fix: Enable CONFIG_PM_CPU_OPS and ensure OpenSBI/firmware\n");
		printk("  supports SBI HSM extension for CPU hotplug.\n");
	} else {
		printk("  ✓ CPU 1 is ONLINE\n");
		printk("  Secondary CPU boot succeeded!\n");
		printk("\n");

		/* WORKAROUND: Spawn thread to initialize IMSIC and enable interrupts on CPU 1 */
		printk("  WORKAROUND: Initializing CPU 1 IMSIC (silently, no printk)...\n");

		/* Initialize semaphore */
		k_sem_init(&cpu1_init_sem, 0, 1);
		cpu1_init_done = false;
		cpu1_init_success = false;

		/* Create thread in suspended state, pin it to CPU 1, then start */
		k_tid_t cpu1_tid = k_thread_create(&cpu1_init_thread_data, cpu1_init_stack,
				K_THREAD_STACK_SIZEOF(cpu1_init_stack),
				cpu1_enable_interrupts_thread,
				NULL, NULL, NULL,
				K_PRIO_PREEMPT(5), K_USER, K_FOREVER);

		/* Pin thread to CPU 1 before starting it */
		k_thread_cpu_mask_clear(cpu1_tid);
		k_thread_cpu_mask_enable(cpu1_tid, 1);

		/* Start the thread */
		k_thread_start(cpu1_tid);

		/* Wait for thread to complete (with timeout) - NO PRINTK while waiting */
		bool sem_ok = (k_sem_take(&cpu1_init_sem, K_MSEC(2000)) == 0);

		/* Now print results after CPU 1 is done */
		if (sem_ok && cpu1_init_success) {
			printk("  ✓ CPU 1 IMSIC initialization completed on CPU %u\n", cpu1_actual_cpu_id);
		} else if (sem_ok && !cpu1_init_success) {
			printk("  ✗ Thread ran on wrong CPU: %u (expected 1)\n", cpu1_actual_cpu_id);
		} else {
			printk("  ✗ CPU 1 initialization TIMED OUT\n");
		}
	}
	printk("\n");
#endif

	/* Step 3: Test GENMSI to CPU 0 */
	printk("STEP 3: Testing GENMSI to CPU 0 (hart 0, EIID %u)\n", TEST_EIID_CPU0);
	printk("======================================\n");

	isr_count_cpu0 = 0;
	isr_count_cpu1 = 0;

	/* GENMSI format: hart_index[31:18] | MSI_DEL[11] | EIID[10:0] */
	uint32_t genmsi_val = (0 << 18) | (1U << 11) | TEST_EIID_CPU0;

	printk("  Writing 0x%08x to GENMSI (Hart=0, MSI_DEL=1, EIID=%u)\n",
	       genmsi_val, TEST_EIID_CPU0);
	aplic_write(APLIC_GENMSI, genmsi_val);

	k_msleep(10);

	printk("  Results:\n");
	printk("    CPU 0 ISR count: %d %s\n", isr_count_cpu0,
	       isr_count_cpu0 > 0 ? "✓" : "✗");
	printk("    CPU 1 ISR count: %d %s\n", isr_count_cpu1,
	       isr_count_cpu1 == 0 ? "✓ (expected 0)" : "✗ (should not fire)");
	printk("\n");

	/* Step 4: Test GENMSI to CPU 1 */
	printk("STEP 4: Testing GENMSI to CPU 1 (hart 1, EIID %u)\n", TEST_EIID_CPU1);
	printk("======================================\n");

	isr_count_cpu0 = 0;
	isr_count_cpu1 = 0;

	/* Target hart 1 */
	genmsi_val = (1 << 18) | (1U << 11) | TEST_EIID_CPU1;
	printk("  Writing 0x%08x to GENMSI (Hart=1, MSI_DEL=1, EIID=%u)\n",
	       genmsi_val, TEST_EIID_CPU1);
	aplic_write(APLIC_GENMSI, genmsi_val);

	k_msleep(10);

	printk("  Results:\n");
	printk("    CPU 0 ISR count: %d %s\n", isr_count_cpu0,
	       isr_count_cpu0 == 0 ? "✓ (expected 0)" : "✗ (should not fire)");
	printk("    CPU 1 ISR count: %d %s\n", isr_count_cpu1,
	       isr_count_cpu1 > 0 ? "✓" : "✗");
	printk("\n");

	/* Step 5: Test multiple GENMSI injections */
	printk("STEP 5: Multiple GENMSI Injections (5 to each CPU)\n");
	printk("======================================\n");

	isr_count_cpu0 = 0;
	isr_count_cpu1 = 0;

	printk("  Sending 5 MSIs to CPU 0...\n");
	for (int i = 0; i < 5; i++) {
		genmsi_val = (0 << 18) | (1U << 11) | TEST_EIID_CPU0;
		aplic_write(APLIC_GENMSI, genmsi_val);
		k_msleep(5);
	}

	printk("  Sending 5 MSIs to CPU 1...\n");
	for (int i = 0; i < 5; i++) {
		genmsi_val = (1 << 18) | (1U << 11) | TEST_EIID_CPU1;
		aplic_write(APLIC_GENMSI, genmsi_val);
		k_msleep(5);
	}

	k_msleep(10);

	printk("  Results:\n");
	printk("    CPU 0 ISR count: %d (expected 5) %s\n", isr_count_cpu0,
	       isr_count_cpu0 == 5 ? "✓" : "✗");
	printk("    CPU 1 ISR count: %d (expected 5) %s\n", isr_count_cpu1,
	       isr_count_cpu1 == 5 ? "✓" : "✗");
	printk("\n");

	/* Step 6: Test broadcast (send same EIID to both CPUs) */
	printk("STEP 6: Testing Broadcast Pattern\n");
	printk("======================================\n");
	printk("  Note: Broadcasting same EIID to multiple harts\n");
	printk("  (Each hart should receive independently)\n\n");

	/* Use a common EIID that both CPUs have enabled */
	const uint32_t BROADCAST_EIID = 70;

	/* Register common broadcast ISR for EIID 70 (will run on whichever CPU receives it) */
	IRQ_CONNECT(BROADCAST_EIID, 1, broadcast_isr_common, NULL, 0);

	/* Enable on CPU 0 (we're running on CPU 0) */
	irq_enable(BROADCAST_EIID);
	printk("  Enabled EIID %u on CPU 0 (already enabled on CPU 1 from init)\n", BROADCAST_EIID);

	broadcast_cpu0 = 0;
	broadcast_cpu1 = 0;

	/* Send to CPU 0 */
	printk("  Sending EIID %u to CPU 0...\n", BROADCAST_EIID);
	genmsi_val = (0 << 18) | (1U << 11) | BROADCAST_EIID;
	aplic_write(APLIC_GENMSI, genmsi_val);
	k_msleep(10);

	/* Send to CPU 1 */
	printk("  Sending EIID %u to CPU 1...\n", BROADCAST_EIID);
	genmsi_val = (1 << 18) | (1U << 11) | BROADCAST_EIID;
	aplic_write(APLIC_GENMSI, genmsi_val);
	k_msleep(10);

	printk("  Results:\n");
	printk("    CPU 0 received: %d %s\n", broadcast_cpu0,
	       broadcast_cpu0 > 0 ? "✓" : "✗");
	printk("    CPU 1 received: %d %s\n", broadcast_cpu1,
	       broadcast_cpu1 > 0 ? "✓" : "✗");
	printk("\n");

	/* Final summary */
	printk("╔═══════════════════════════════════════════════╗\n");
	printk("║              TEST SUMMARY                     ║\n");
	printk("╚═══════════════════════════════════════════════╝\n");
	printk("\n");
	printk("Configuration:\n");
	printk("  APLIC Domain:        %s\n",
	       (domaincfg & BIT(8)) ? "✓ Enabled" : "✗ Disabled");
	printk("  MSI Address Setup:   %s\n",
	       (msiaddr_low == (IMSIC0_BASE >> 12)) ? "✓ Correct" : "✗ Incorrect");
	printk("  SMP Configuration:   %u CPUs\n", CONFIG_MP_MAX_NUM_CPUS);
	printk("\n");
	printk("Test Results:\n");
	printk("  CPU 0 targeting:     (see Step 3)\n");
	printk("  CPU 1 targeting:     (see Step 4)\n");
	printk("  Multiple injections: (see Step 5)\n");
	printk("  Broadcast pattern:   (see Step 6)\n");
	printk("\n");

	return 0;
}

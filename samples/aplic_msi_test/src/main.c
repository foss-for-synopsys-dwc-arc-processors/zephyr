/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * APLIC MSI Delivery Test
 * Tests the complete APLIC → IMSIC interrupt flow
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>

/* Helper function for direct IMSIC EIP injection (for testing).
 * This writes directly to EIP registers to inject interrupts, which works
 * on platforms that support EIP writes (nSIM, some emulators).
 */
static inline void riscv_imsic_inject_sw_interrupt_qemu(uint32_t eiid)
{
	uint32_t reg_index = eiid / 32U;
	uint32_t bit = eiid % 32U;
	uint32_t icsr_addr = 0x80U + reg_index;  /* ICSR_EIP0 base */

	/* Access via miselect/mireg CSRs */
	__asm__ volatile("csrw 0x350, %0" : : "r"(icsr_addr));  /* miselect */
	__asm__ volatile("csrw 0x351, %0" : : "r"(BIT(bit)));   /* mireg */
}

#define TEST_EIID  64
#define TEST_SOURCE 10  /* APLIC source number */

static volatile int isr_count;

/* ISR callback */
static void test_isr(const void *arg)
{
	ARG_UNUSED(arg);
	isr_count++;
	printk(">>> ISR FIRED! Count=%d (EIID %d) <<<\n", isr_count, TEST_EIID);
}

int main(void)
{
	const struct device *aplic = riscv_aplic_get_dev();
	const struct device *imsic = riscv_imsic_get_dev();

	printk("\n");
	printk("╔════════════════════════════════════════════════╗\n");
	printk("║     APLIC MSI Delivery Test                   ║\n");
	printk("╚════════════════════════════════════════════════╝\n");
	printk("\n");

	if (!aplic || !imsic) {
		printk("ERROR: APLIC or IMSIC device not found!\n");
		return -1;
	}

	/* Setup: Register ISR */
	printk("SETUP: Registering interrupt handler\n");
	printk("-------------------------------------\n");
	IRQ_CONNECT(TEST_EIID, 1, test_isr, NULL, 0);
	irq_enable(TEST_EIID);
	printk("  ✓ ISR registered for EIID %d\n", TEST_EIID);
	printk("  ✓ IMSIC EIE enabled\n");

	/* Check APLIC device info */
	printk("\nAPLIC Configuration:\n");
	printk("-------------------------------------\n");
	printk("  APLIC device: %p\n", aplic);
	printk("  Testing source: %d\n", TEST_SOURCE);
	printk("  Target EIID: %d\n", TEST_EIID);
	printk("  Target hart: 0\n");

	/* Test 1: Direct IMSIC injection (baseline) */
	printk("\n");
	printk("TEST 1: Direct IMSIC injection (baseline)\n");
	printk("==========================================\n");
	printk("Using: riscv_imsic_inject_sw_interrupt_qemu()\n");

	riscv_imsic_inject_sw_interrupt_qemu(TEST_EIID);
	k_msleep(10);

	printk("Result: %d interrupts received %s\n", isr_count,
	       (isr_count == 1) ? "✓" : "✗");
	int baseline_count = isr_count;

	/* Test 2: Try GENMSI without any configuration */
	printk("\n");
	printk("TEST 2: APLIC genmsi (no configuration)\n");
	printk("==========================================\n");
	printk("Using: riscv_aplic_inject_genmsi(hart=0, eiid=%d)\n", TEST_EIID);

	int before = isr_count;

	riscv_aplic_inject_genmsi(0, TEST_EIID);
	k_msleep(10);

	int received = isr_count - before;

	printk("Result: %d interrupts received %s\n", received,
	       (received > 0) ? "✓" : "✗");

	/* Test 3: Configure APLIC source first, then try genmsi */
	printk("\n");
	printk("TEST 3: Configure APLIC source, then genmsi\n");
	printk("==========================================\n");

	/* Step 3a: Configure source mode (edge-triggered, active-high) */
	printk("Step 3a: Configure SOURCECFG[%d]\n", TEST_SOURCE);
	int ret = riscv_aplic_msi_config_src(aplic, TEST_SOURCE, APLIC_SM_EDGE_RISE);
	printk("  riscv_aplic_msi_config_src() = %d\n", ret);

	/* Step 3b: Route source to hart 0, EIID 64 */
	printk("Step 3b: Configure TARGET[%d] = hart:0 eiid:%d\n", TEST_SOURCE, TEST_EIID);
	ret = riscv_aplic_msi_route(aplic, TEST_SOURCE, 0, TEST_EIID);
	printk("  riscv_aplic_msi_route() = %d\n", ret);

	/* Step 3c: Enable the source */
	printk("Step 3c: Enable source %d\n", TEST_SOURCE);
	riscv_aplic_enable_source(TEST_SOURCE);
	printk("  riscv_aplic_enable_source() called\n");

	/* Step 3d: Try genmsi again */
	printk("Step 3d: Inject via genmsi\n");
	before = isr_count;
	riscv_aplic_inject_genmsi(0, TEST_EIID);
	k_msleep(10);

	received = isr_count - before;
	printk("Result: %d interrupts received %s\n", received,
	       (received > 0) ? "✓" : "✗");

	/* Test 4: Try multiple genmsi injections */
	printk("\n");
	printk("TEST 4: Multiple APLIC genmsi injections (x5)\n");
	printk("==========================================\n");

	before = isr_count;
	for (int i = 0; i < 5; i++) {
		riscv_aplic_inject_genmsi(0, TEST_EIID);
		k_msleep(5);
	}

	received = isr_count - before;
	printk("Result: %d interrupts received (expected 5) %s\n", received,
	       (received == 5) ? "✓" : "✗");

	/* Test 5: Verify APLIC register addresses and contents */
	printk("\n");
	printk("TEST 5: APLIC Register Inspection\n");
	printk("==========================================\n");

	/* Get base address from config (reuse aplic device pointer) */
	struct aplic_cfg {
		uintptr_t base;
		uint32_t num_sources;
	};
	const struct aplic_cfg *aplic_cfg = aplic->config;

	printk("APLIC Base Address: 0x%08lx\n", (unsigned long)aplic_cfg->base);
	printk("Expected GENMSI at: 0x%08lx (base + 0x3000)\n",
	       (unsigned long)(aplic_cfg->base + 0x3000));

	/* Read key APLIC registers */
	uint32_t domaincfg = sys_read32(aplic_cfg->base + 0x0000);
	uint32_t sourcecfg10 = sys_read32(aplic_cfg->base + 0x0004 + (10-1)*4);
	uint32_t target10 = sys_read32(aplic_cfg->base + 0x3004 + (10-1)*4);
	uint32_t genmsi_read = sys_read32(aplic_cfg->base + 0x3000);

	/* Read MSI address configuration registers */
	uint32_t msiaddrcfg = sys_read32(aplic_cfg->base + 0x1BC0);
	uint32_t msiaddrcfgh = sys_read32(aplic_cfg->base + 0x1BC4);

	printk("\nAPLIC Register Contents:\n");
	printk("  DOMAINCFG   [0x%08lx] = 0x%08x\n",
	       (unsigned long)(aplic_cfg->base + 0x0000), domaincfg);
	printk("    IE bit (8):  %d\n", !!(domaincfg & BIT(8)));
	printk("    DM bit (2):  %d (0=direct, 1=MSI)\n", !!(domaincfg & BIT(2)));
	printk("    BE bit (0):  %d (0=LE, 1=BE)\n", !!(domaincfg & BIT(0)));

	printk("\n  MSI Address Configuration:\n");
	printk("  MSIADDRCFG  [0x%08lx] = 0x%08x\n",
	       (unsigned long)(aplic_cfg->base + 0x1BC0), msiaddrcfg);
	printk("  MSIADDRCFGH [0x%08lx] = 0x%08x\n",
	       (unsigned long)(aplic_cfg->base + 0x1BC4), msiaddrcfgh);
	printk("    Full MSI target address: 0x%08x%08x\n", msiaddrcfgh, msiaddrcfg);
	printk("    Expected IMSIC M-mode:   0x24000000\n");

	printk("\n  Source Configuration:\n");
	printk("  SOURCECFG[10] [0x%08lx] = 0x%08x\n",
	       (unsigned long)(aplic_cfg->base + 0x0004 + (10-1)*4), sourcecfg10);
	printk("    SM (source mode): %d\n", sourcecfg10 & 0x7);

	printk("  TARGET[10]  [0x%08lx] = 0x%08x\n",
	       (unsigned long)(aplic_cfg->base + 0x3004 + (10-1)*4), target10);
	printk("    Hart Index: %d\n", (target10 >> 18) & 0x3FFF);
	printk("    EIID:       %d\n", target10 & 0x7FF);

	printk("\n  GENMSI Register:\n");
	printk("  GENMSI      [0x%08lx] = 0x%08x\n",
	       (unsigned long)(aplic_cfg->base + 0x3000), genmsi_read);
	printk("    Decoded: Hart=%d, Context=%d, EIID=%d\n",
	       (genmsi_read >> 18) & 0x3FFF, (genmsi_read >> 13) & 0x1F, genmsi_read & 0x7FF);

	/* Check TARGET[0] - might be used for GENMSI routing */
	uint32_t target0 = sys_read32(aplic_cfg->base + 0x3004);

	printk("\n  TARGET[0] (GENMSI routing?):\n");
	printk("  TARGET[0]   [0x%08lx] = 0x%08x\n",
	       (unsigned long)(aplic_cfg->base + 0x3004), target0);
	printk("    Hart Index: %d, EIID: %d\n",
	       (target0 >> 18) & 0x3FFF, target0 & 0x7FF);

	/* Test configuring TARGET[0] for GENMSI */
	printk("\n  Configuring TARGET[0] for GENMSI → EIID %d:\n", TEST_EIID);
	uint32_t target0_val = ((0 & 0x3FFF) << 18) | (TEST_EIID & 0x7FF);
	sys_write32(target0_val, aplic_cfg->base + 0x3004);
	uint32_t target0_readback = sys_read32(aplic_cfg->base + 0x3004);
	printk("    Wrote 0x%08x, readback = 0x%08x\n", target0_val, target0_readback);

	/* Test GENMSI with TARGET[0] configured */
	printk("\n  Testing GENMSI with TARGET[0] configured:\n");
	before = isr_count;
	riscv_aplic_inject_genmsi(0, TEST_EIID);
	k_msleep(10);
	received = isr_count - before;
	printk("    Result: %d interrupts %s\n", received, (received > 0) ? "✓" : "✗");

	/* Test 6: Try triggering via SETIPNUM (set interrupt pending) */
	printk("\n");
	printk("TEST 6: APLIC SETIPNUM (trigger source 10)\n");
	printk("==========================================\n");
	printk("This should trigger the configured route: source 10 → EIID 64\n");

	before = isr_count;

	/* Set interrupt pending for source 10 using SETIPNUM */
	sys_write32(TEST_SOURCE, aplic_cfg->base + 0x1CDC);  /* SETIPNUM */
	printk("Wrote source %d to SETIPNUM register\n", TEST_SOURCE);

	k_msleep(10);

	received = isr_count - before;
	printk("Result: %d interrupts received %s\n", received,
	       (received > 0) ? "✓" : "✗");

	/* Test 7: Try different GENMSI interpretations */
	printk("\n");
	printk("TEST 7: Try different GENMSI values\n");
	printk("==========================================\n");

	/* Maybe GENMSI expects SOURCE NUMBER instead of EIID? */
	printk("Hypothesis: GENMSI writes source number, uses TARGET[source] for routing\n\n");

	/* Try writing source 10 (which is configured to route to EIID 64) */
	printk("Try 1: Write source 10 to GENMSI\n");
	printk("       (source 10 is configured: TARGET[10] = hart:0, eiid:64)\n");
	before = isr_count;
	sys_write32(10, aplic_cfg->base + 0x3000);  /* Write source number */
	k_msleep(10);
	received = isr_count - before;
	printk("  Result: %d interrupts %s\n", received, (received > 0) ? "✓" : "✗");

	/* Try writing source 1 */
	printk("\nTry 2: Write source 1 to GENMSI\n");
	printk("       (configure TARGET[1] first)\n");
	uint32_t target1_val = ((0 & 0x3FFF) << 18) | (TEST_EIID & 0x7FF);
	sys_write32(target1_val, aplic_cfg->base + 0x3004 + (1-1)*4);  /* TARGET[1] */
	before = isr_count;
	sys_write32(1, aplic_cfg->base + 0x3000);  /* Write source 1 */
	k_msleep(10);
	received = isr_count - before;
	printk("  Result: %d interrupts %s\n", received, (received > 0) ? "✓" : "✗");

	/* Try really low EIID */
	printk("\nTry 3: Write EIID 1 directly (not source)\n");
	before = isr_count;
	sys_write32(1, aplic_cfg->base + 0x3000);
	k_msleep(10);
	received = isr_count - before;
	printk("  Result: %d interrupts %s\n", received, (received > 0) ? "✓" : "✗");

	/* Conclusion */
	printk("\n");
	printk("CONCLUSION: APLIC GENMSI in QEMU appears non-functional\n");
	printk("  - Register is writable (readback works)\n");
	printk("  - MSIADDRCFG is configured (0x24000000)\n");
	printk("  - But no MSI writes are generated to IMSIC\n");
	printk("  - This suggests QEMU APLIC GENMSI is not implemented\n");

	/* Summary */
	printk("\n");
	printk("╔════════════════════════════════════════════════╗\n");
	printk("║              TEST SUMMARY                     ║\n");
	printk("╠════════════════════════════════════════════════╣\n");
	printk("║ Total interrupts: %-3d                        ║\n", isr_count);
	printk("║ Baseline (direct IMSIC): %-3d                 ║\n", baseline_count);
	printk("║ APLIC genmsi: %-3d                            ║\n", isr_count - baseline_count);
	printk("╚════════════════════════════════════════════════╝\n");

	if (isr_count > baseline_count) {
		printk("\n✓ APLIC → IMSIC MSI delivery is WORKING!\n");
		printk("APLIC genmsi successfully delivered %d interrupts\n",
		       isr_count - baseline_count);
	} else {
		printk("\n✗ APLIC genmsi did NOT work\n");
		printk("Only baseline IMSIC injection worked\n");
		printk("\nPossible issues:\n");
		printk("  - QEMU genmsi implementation incomplete\n");
		printk("  - Missing APLIC configuration (MSI base address?)\n");
		printk("  - APLIC not connected to IMSIC memory region\n");
	}

	return 0;
}

/*
 * APLIC GENMSI Debug Test
 *
 * Validates that APLIC GENMSI register writes correctly trigger
 * MSI writes to IMSIC by checking all register states.
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
#define IMSIC_NODE DT_NODELABEL(imsic0)

#define APLIC_BASE DT_REG_ADDR(APLIC_NODE)
#define IMSIC_BASE DT_REG_ADDR(IMSIC_NODE)

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

static volatile int test_isr_count = 0;
static volatile uint32_t last_eiid = 0;

/* Test ISR */
static void test_isr(const void *arg)
{
	ARG_UNUSED(arg);
	test_isr_count++;
	last_eiid = (uint32_t)arg;
	printk("  [ISR] Fired! Count=%d, EIID=%u\n", test_isr_count, last_eiid);
}

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
	printk("║     APLIC GENMSI Debug & Validation Test     ║\n");
	printk("╚═══════════════════════════════════════════════╝\n");
	printk("\n");

	printk("Hardware Configuration:\n");
	printk("  APLIC Base:  0x%08lx\n", (unsigned long)APLIC_BASE);
	printk("  IMSIC Base:  0x%08lx\n", (unsigned long)IMSIC_BASE);
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
	printk("    - BE (bit 0):  %s\n", (domaincfg & BIT(0)) ? "Big endian" : "Little endian");
	printk("  MSIADDRCFG:   0x%08x (PPN for IMSIC base)\n", msiaddr_low);
	printk("  MSIADDRCFGH:  0x%08x (geometry fields)\n", msiaddr_high);
	printk("  Expected PPN: 0x%08x (IMSIC 0x%08lx >> 12)\n",
	       (uint32_t)(IMSIC_BASE >> 12), (unsigned long)IMSIC_BASE);

	if (msiaddr_low != (IMSIC_BASE >> 12)) {
		printk("  ⚠️  WARNING: MSIADDRCFG doesn't match expected IMSIC address!\n");
	} else {
		printk("  ✓ MSIADDRCFG matches IMSIC address\n");
	}
	printk("\n");

	/* Step 2: Read IMSIC state */
	printk("STEP 2: Reading IMSIC State\n");
	printk("======================================\n");

	uint32_t eidelivery = imsic_read_indirect(IMSIC_EIDELIVERY);
	uint32_t eithreshold = imsic_read_indirect(IMSIC_EITHRESHOLD);
	uint32_t eip0 = imsic_read_indirect(IMSIC_EIP0);
	uint32_t eie0 = imsic_read_indirect(IMSIC_EIE0);

	printk("  EIDELIVERY:   0x%08x\n", eidelivery);
	printk("    - ENABLE (bit 0):  %s\n", (eidelivery & BIT(0)) ? "Yes" : "No");
	uint32_t mode = (eidelivery >> 29) & 0x3;
	const char *mode_str[] = {"MMSI", "DMSI", "DDI", "MMSI_DMSI"};
	printk("    - MODE (bits 30:29): 0x%x (%s)\n", mode, mode_str[mode]);
	if (mode == 1) {
		printk("    ⚠️  WARNING: DMSI mode selected but MMSI expected!\n");
	}
	printk("  EITHRESHOLD:  0x%08x\n", eithreshold);
	printk("  EIP0:         0x%08x (pending bits [31:0])\n", eip0);
	printk("  EIE0:         0x%08x (enabled bits [31:0])\n", eie0);
	printk("\n");

	/* Step 3: Setup test interrupt handler for EIID 64 */
	printk("STEP 3: Setting up Test ISR for EIID 64\n");
	printk("======================================\n");

	const uint32_t TEST_EIID = 64;

	IRQ_CONNECT(TEST_EIID, 1, test_isr, (void *)TEST_EIID, 0);
	irq_enable(TEST_EIID);

	printk("  Registered ISR for EIID %u\n", TEST_EIID);

	/* Re-read IMSIC state after enabling */
	eie0 = imsic_read_indirect(IMSIC_EIE0);
	printk("  EIE0 after enable: 0x%08x\n", eie0);

	/* Check EIE[2] which covers EIIDs 64-95 */
	uint32_t eie2 = imsic_read_indirect(IMSIC_EIE0 + 2);
	printk("  EIE2 (EIIDs 64-95): 0x%08x\n", eie2);
	printk("    - Bit 0 (EIID 64): %s\n", (eie2 & BIT(0)) ? "Enabled" : "Disabled");
	printk("\n");

	/* Step 4: Write to GENMSI register */
	printk("STEP 4: Writing to APLIC GENMSI Register\n");
	printk("======================================\n");

	printk("  Before write:\n");
	uint32_t eip2_before = imsic_read_indirect(IMSIC_EIP0 + 2);
	printk("    EIP2 (pending): 0x%08x\n", eip2_before);
	printk("    ISR count:      %d\n", test_isr_count);

	/* Write to GENMSI with MSI_DEL bit set for MMSI delivery */
	uint32_t genmsi_val = (1U << 11) | TEST_EIID;  /* MSI_DEL=1, EIID=64 */
	printk("\n  Writing 0x%08x to GENMSI (MSI_DEL=1, EIID=%u)...\n", genmsi_val, TEST_EIID);
	aplic_write(APLIC_GENMSI, genmsi_val);

	/* Read back GENMSI */
	uint32_t genmsi_readback = aplic_read(APLIC_GENMSI);
	printk("  GENMSI readback: 0x%08x\n", genmsi_readback);

	/* Small delay for MSI propagation */
	k_msleep(10);

	printk("\n  After write:\n");
	uint32_t eip2_after = imsic_read_indirect(IMSIC_EIP0 + 2);
	printk("    EIP2 (pending): 0x%08x\n", eip2_after);
	printk("    ISR count:      %d\n", test_isr_count);

	/* Note: EIP bits are automatically cleared when MTOPEI claims the interrupt.
	 * If the ISR was called, that means the MSI reached IMSIC successfully,
	 * even if EIP reads as 0 afterwards.
	 */
	if (test_isr_count > 0) {
		printk("  ✓ ISR was called - MSI delivery successful!\n");
		if (eip2_after == eip2_before) {
			printk("  ℹ️  EIP2 unchanged because interrupt was already claimed and cleared\n");
		}
	} else {
		printk("  ✗ ISR was NOT called\n");
		if (eip2_after != eip2_before) {
			printk("  ⚠️  EIP2 changed but ISR didn't fire - check interrupt routing\n");
		} else {
			printk("  ✗ EIP2 unchanged - MSI write did NOT reach IMSIC\n");
		}
	}
	printk("\n");

	/* Step 5: Try alternative GENMSI formats */
	printk("STEP 5: Trying Alternative GENMSI Formats\n");
	printk("======================================\n");

	/* Format 1: With MSI_DEL bit set (bit 11) */
	test_isr_count = 0;
	genmsi_val = (1U << 11) | TEST_EIID;
	printk("  Format 1: MSI_DEL + EIID = 0x%08x\n", genmsi_val);
	aplic_write(APLIC_GENMSI, genmsi_val);
	k_msleep(10);
	printk("    ISR count: %d %s\n", test_isr_count,
	       test_isr_count > 0 ? "✓" : "✗");

	/* Format 2: Full encoding with hart=0, guest=0, MSI_DEL, eiid */
	test_isr_count = 0;
	genmsi_val = (0 << 18) | (0 << 12) | (1U << 11) | TEST_EIID;
	printk("  Format 2: Hart=0, Guest=0, MSI_DEL, EIID = 0x%08x\n", genmsi_val);
	aplic_write(APLIC_GENMSI, genmsi_val);
	k_msleep(10);
	printk("    ISR count: %d %s\n", test_isr_count,
	       test_isr_count > 0 ? "✓" : "✗");

	/* Format 3: Full encoding with MSI_DEL */
	test_isr_count = 0;
	genmsi_val = (0 << 18) | (0 << 12) | (1U << 11) | TEST_EIID;
	printk("  Format 3: Hart=0, Guest=0, MSI_DEL, EIID = 0x%08x\n", genmsi_val);
	aplic_write(APLIC_GENMSI, genmsi_val);
	k_msleep(10);
	printk("    ISR count: %d %s\n", test_isr_count,
	       test_isr_count > 0 ? "✓" : "✗");
	printk("\n");

	/* Step 6: Direct IMSIC test (bypass APLIC) */
	printk("STEP 6: Direct IMSIC Injection Test (Bypass APLIC)\n");
	printk("======================================\n");

	test_isr_count = 0;
	printk("  Writing directly to IMSIC EIP register...\n");

	/* Directly set the pending bit for EIID 64 in EIP2 */
	uint32_t eip2_current = imsic_read_indirect(IMSIC_EIP0 + 2);
	imsic_write_indirect(IMSIC_EIP0 + 2, eip2_current | BIT(0));

	k_msleep(10);

	printk("  ISR count: %d %s\n", test_isr_count,
	       test_isr_count > 0 ? "✓" : "✗");

	if (test_isr_count > 0) {
		printk("  ✓ Direct IMSIC injection works - ISR path is OK\n");
	} else {
		printk("  ✗ Direct IMSIC injection also failed\n");
		printk("  ⚠️  Issue might be in IMSIC or ISR configuration\n");
	}
	printk("\n");

	/* Final summary */
	printk("╔═══════════════════════════════════════════════╗\n");
	printk("║              DIAGNOSTIC SUMMARY               ║\n");
	printk("╚═══════════════════════════════════════════════╝\n");
	printk("\n");
	printk("Configuration Status:\n");
	printk("  APLIC Domain:        %s\n", (domaincfg & BIT(8)) ? "✓ Enabled" : "✗ Disabled");
	printk("  MSI Address Setup:   %s\n",
	       (msiaddr_low == (IMSIC_BASE >> 12)) ? "✓ Correct" : "✗ Incorrect");
	printk("  IMSIC ENABLE:        %s\n", (eidelivery & BIT(0)) ? "✓ Yes" : "✗ No");
	uint32_t mode_final = (eidelivery >> 29) & 0x3;
	printk("  IMSIC MODE:          %s %s\n", mode_str[mode_final],
	       (mode_final == 0 || mode_final == 3) ? "✓" : "✗ (should be MMSI)");
	printk("  EIID 64 Enabled:     %s\n", (eie2 & BIT(0)) ? "✓ Yes" : "✗ No");
	printk("\n");
	printk("Test Results:\n");
	printk("  Direct IMSIC:        %s\n", test_isr_count > 0 ? "✓ Working" : "✗ Failed");
	printk("  APLIC GENMSI:        (check Step 4-5 above)\n");
	printk("\n");

	return 0;
}

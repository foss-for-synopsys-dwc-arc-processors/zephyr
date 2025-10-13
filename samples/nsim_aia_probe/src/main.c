/*
 * nSIM RTIA CSR Access Test
 *
 * Test if nSIM implements IMSIC CSRs via miselect/mireg
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/irq.h>

#define UART_BASE 0xF0000000
#define UART_IER  (0x01 << 2)
#define UART_LSR  (0x05 << 2)
#define UART_RBR  (0x00 << 2)

/* CSR addresses */
#define CSR_MISELECT  0x350
#define CSR_MIREG     0x351
#define CSR_MTOPEI    0x35C

/* Indirect CSR addresses */
#define ICSR_EIDELIVERY  0x70
#define ICSR_EITHRESH    0x72
#define ICSR_EIE0        0xC0
#define ICSR_EIE1        0xC1
#define ICSR_EIP0        0x80
#define ICSR_EIP1        0x81

/* EIDELIVERY bits */
#define EIDELIVERY_ENABLE     (1U << 0)
#define EIDELIVERY_MODE_MMSI  (2U << 29)

static volatile int uart_isr_count = 0;

static inline uint32_t read_csr(uint32_t csr)
{
	uint32_t value;
	__asm__ volatile("csrr %0, %1" : "=r"(value) : "i"(csr));
	return value;
}

static inline void write_csr(uint32_t csr, uint32_t value)
{
	__asm__ volatile("csrw %0, %1" : : "i"(csr), "r"(value));
}

static inline uint32_t read_imsic_csr(uint32_t icsr_addr)
{
	uint32_t value;
	__asm__ volatile("csrw 0x350, %0" : : "r"(icsr_addr));  /* miselect */
	__asm__ volatile("csrr %0, 0x351" : "=r"(value));       /* mireg */
	return value;
}

static inline void write_imsic_csr(uint32_t icsr_addr, uint32_t value)
{
	__asm__ volatile("csrw 0x350, %0" : : "r"(icsr_addr));  /* miselect */
	__asm__ volatile("csrw 0x351, %0" : : "r"(value));      /* mireg */
}

void uart_irq_handler(const void *arg)
{
	ARG_UNUSED(arg);
	uart_isr_count++;
	printk("[ISR %d] UART interrupt!\n", uart_isr_count);

	/* Read and clear */
	uint8_t lsr = sys_read8(UART_BASE + UART_LSR);
	if (lsr & 0x01) {
		uint8_t ch = sys_read8(UART_BASE + UART_RBR);
		printk("[ISR] Got char: 0x%02x '%c'\n", ch,
		       (ch >= 32 && ch < 127) ? ch : '.');
	}
}

int main(void)
{
	/* Register ISR for EIID 42 (UART IRQ might use identity mapping) */
	IRQ_CONNECT(42, 0, uart_irq_handler, NULL, 0);

	printk("\n=== nSIM RTIA CSR Access Test ===\n\n");

	/* Test 1: Try reading miselect CSR */
	printk("Test 1: Reading miselect CSR...\n");
	uint32_t miselect_val;

	__asm__ volatile("csrr %0, 0x350" : "=r"(miselect_val));
	printk("  miselect (0x350) = 0x%08x\n\n", miselect_val);

	/* Test 1.5: Try reading mireg AFTER setting miselect */
	printk("Test 1.5: Reading mireg (0x351) AFTER setting miselect to 0x70...\n");
	__asm__ volatile("csrw 0x350, %0" : : "r"(ICSR_EIDELIVERY));  /* Write 0x70 to miselect */
	uint32_t mireg_val;
	__asm__ volatile("csrr %0, 0x351" : "=r"(mireg_val));
	printk("  mireg (0x351) = 0x%08x (after miselect=0x70)\n\n", mireg_val);

	/* Test 2: Try reading EIDELIVERY via indirect access */
	printk("Test 2: Reading EIDELIVERY (0x70) via miselect/mireg...\n");
	uint32_t eidelivery = read_imsic_csr(ICSR_EIDELIVERY);
	printk("  EIDELIVERY = 0x%08x\n", eidelivery);
	printk("    ENABLE bit[0] = %u\n", eidelivery & 1);
	printk("    MODE bits[30:29] = 0x%x\n\n", (eidelivery >> 29) & 0x3);

	/* Test 3: Try reading EITHRESHOLD */
	printk("Test 3: Reading EITHRESHOLD (0x72)...\n");
	uint32_t eithresh = read_imsic_csr(ICSR_EITHRESH);
	printk("  EITHRESHOLD = 0x%08x\n\n", eithresh);

	/* Test 4: Try writing EIDELIVERY to enable MMSI */
	printk("Test 4: Writing EIDELIVERY to enable MMSI mode...\n");
	uint32_t eidelivery_new = EIDELIVERY_ENABLE | EIDELIVERY_MODE_MMSI;
	printk("  Writing: 0x%08x (ENABLE=1, MODE=MMSI)\n", eidelivery_new);
	write_imsic_csr(ICSR_EIDELIVERY, eidelivery_new);

	/* Read back */
	eidelivery = read_imsic_csr(ICSR_EIDELIVERY);
	printk("  Readback: 0x%08x\n", eidelivery);
	printk("    ENABLE bit[0] = %u\n", eidelivery & 1);
	printk("    MODE bits[30:29] = 0x%x\n\n", (eidelivery >> 29) & 0x3);

	/* Test 5: Set EITHRESHOLD to 0 */
	printk("Test 5: Setting EITHRESHOLD to 0...\n");
	write_imsic_csr(ICSR_EITHRESH, 0);
	eithresh = read_imsic_csr(ICSR_EITHRESH);
	printk("  EITHRESHOLD readback = 0x%08x\n\n", eithresh);

	/* Test 6: Read EIE1 (EIID 32-63 enable) */
	printk("Test 6: Reading EIE1 (interrupt enable for EIID 32-63)...\n");
	uint32_t eie1 = read_imsic_csr(ICSR_EIE1);
	printk("  EIE1 (0xC1) = 0x%08x\n\n", eie1);

	/* Test 7: Enable EIID 42 in EIE1 (for UART IRQ) */
	printk("Test 7: Enabling EIID 42 (bit 10 of EIE1 for IRQ 32-63)...\n");
	write_imsic_csr(ICSR_EIE1, eie1 | (1U << (42 - 32)));  /* EIID 42, bit 10 of EIE1 */
	eie1 = read_imsic_csr(ICSR_EIE1);
	printk("  EIE1 readback = 0x%08x (bit 10 = %u)\n\n", eie1, (eie1 >> 10) & 1);

	/* Test 8: Check if APLIC is accessible */
	printk("Test 8: Probing APLIC at 0xF8000000...\n");

	/* APLIC is at 0xF8000000 */
	#define APLIC_BASE 0xF8000000
	#define APLIC_DOMAINCFG    0x0000

	printk("  Reading APLIC DOMAINCFG...\n");
	uint32_t domaincfg = sys_read32(APLIC_BASE + APLIC_DOMAINCFG);
	printk("  APLIC DOMAINCFG = 0x%08x\n", domaincfg);

	if (domaincfg == 0 || domaincfg == 0xFFFFFFFF) {
		printk("  ⚠ APLIC may not be accessible or configured\n\n");
		printk("  Skipping APLIC configuration, using software MSI injection instead\n\n");

		/* Just enable EIID 32 in IMSIC and use software injection */
		printk("Test 8b: Testing software MSI injection to EIID 32...\n");

		/* EIID 42 already enabled in Test 7, skip irq_enable() */

		/* Enable MSTATUS.MIE and MIE.MEIE */
		__asm__ volatile("csrsi 0x300, 0x8");  /* MSTATUS.MIE */
		uint32_t meie_bit = (1 << 11);
		__asm__ volatile("csrrs x0, 0x304, %0" : : "r"(meie_bit)); /* MIE.MEIE */

		printk("  Global interrupts enabled\n");

		/* Inject software interrupt to EIID 32 */
		printk("  Injecting SW interrupt to EIID 32...\n");
		write_imsic_csr(ICSR_EIP1, 0x00000001);  /* Set bit 0 of EIP1 = EIID 32 */

		k_msleep(10);
		printk("  After SW injection: uart_isr_count = %d\n\n", uart_isr_count);

		/* Skip UART test since APLIC not working */
		printk("\n=== Test complete ===\n");
		printk("Note: APLIC not accessible on nSIM, used SW MSI injection\n");
		return 0;
	}

	printk("  ✓ APLIC is accessible\n\n");

	/* Test 9: APLIC is pre-configured via props (RO registers) */
	printk("Test 9: APLIC pre-configured via props (assuming source 42 → EIID 42)...\n");
	printk("  Note: nSIM APLIC uses RO registers configured via props file\n");
	printk("  Assuming identity mapping: IRQ 42 → EIID 42\n\n");

	/* Enable EIID 42 interrupt routing - do it manually via CSR */
	printk("Test 9b: Enabling EIID 42 manually in IMSIC...\n");
	/* EIID 42 is bit 10 of EIE1 (already done in Test 7) */
	printk("  EIID 42 already enabled in Test 7\n");

	/* Enable MSTATUS.MIE and MIE.MEIE */
	__asm__ volatile("csrsi 0x300, 0x8");  /* MSTATUS.MIE */
	uint32_t meie_bit = (1 << 11);
	__asm__ volatile("csrrs x0, 0x304, %0" : : "r"(meie_bit)); /* MIE.MEIE */
	printk("  ISR registered, global interrupts enabled\n\n");

	/* Test 10: Enable UART RX interrupts */
	printk("Test 10: Enabling UART RX interrupts...\n");
	sys_write8(0x01, UART_BASE + UART_IER);
	printk("  UART IER = 0x01\n\n");

	/* Test 11: Check mtopei CSR */
	printk("Test 11: Reading mtopei (0x35C)...\n");
	uint32_t mtopei;
	__asm__ volatile("csrr %0, 0x35c" : "=r"(mtopei));
	printk("  mtopei = 0x%08x\n", mtopei);
	printk("  EIID = %u, Priority = %u\n\n", mtopei & 0x7FF, (mtopei >> 16) & 0xFF);

	printk("=== Waiting for UART input (3 seconds) ===\n\n");
	k_msleep(3000);

	printk("\nResult: uart_isr_count = %d\n", uart_isr_count);

	/* Final CSR state */
	printk("\nFinal state:\n");
	uint32_t eip1 = read_imsic_csr(ICSR_EIP1);
	printk("  EIP1 (pending) = 0x%08x\n", eip1);
	eie1 = read_imsic_csr(ICSR_EIE1);
	printk("  EIE1 (enable) = 0x%08x\n", eie1);
	eidelivery = read_imsic_csr(ICSR_EIDELIVERY);
	printk("  EIDELIVERY = 0x%08x\n", eidelivery);

	printk("\n=== Test complete ===\n");
	return 0;
}

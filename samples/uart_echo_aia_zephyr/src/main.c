/*
 * UART Echo Demo using AIA with Zephyr Abstractions
 *
 * This demo shows how to use RISC-V AIA (APLIC + IMSIC) with Zephyr's
 * standard APIs:
 *
 * 1. Use device tree macros for configuration
 * 2. Use IRQ_CONNECT/irq_enable for ISR registration
 * 3. Use AIA APIs (riscv_aplic_msi_route) for MSI routing
 * 4. Manual UART register handling (for simplicity and reliability)
 *
 * This demonstrates AIA interrupt delivery while keeping UART handling simple.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>

/* Get UART configuration from device tree */
#define UART_NODE DT_CHOSEN(zephyr_console)
#define UART_BASE DT_REG_ADDR(UART_NODE)
#define UART_IRQ_NUM DT_IRQ(UART_NODE, irq)
#define UART_IRQ_PRIORITY DT_IRQ(UART_NODE, priority)

/* 16550 UART register offsets */
#define UART_RBR 0x00  /* Receiver Buffer Register (read) */
#define UART_THR 0x00  /* Transmitter Holding Register (write) */
#define UART_IER 0x01  /* Interrupt Enable Register */
#define UART_LSR 0x05  /* Line Status Register */

/* UART_IER bits */
#define UART_IER_RDI 0x01  /* Receiver Data Interrupt */

/* UART_LSR bits */
#define UART_LSR_DR 0x01   /* Data Ready */
#define UART_LSR_THRE 0x20 /* Transmitter Holding Register Empty */

/* EIID to use for UART interrupt */
#define UART_EIID 32

/* Statistics */
static volatile int rx_char_count = 0;
static volatile int isr_entry_count = 0;

/* Helper functions for UART register access */
static inline uint8_t uart_read_reg(uint32_t offset)
{
	return sys_read8(UART_BASE + offset);
}

static inline void uart_write_reg(uint32_t offset, uint8_t value)
{
	sys_write8(value, UART_BASE + offset);
}

/**
 * UART ISR - called when IMSIC delivers MSI to CPU
 * Handles UART RX and echoes characters back
 */
static void uart_eiid_isr(const void *arg)
{
	ARG_UNUSED(arg);

	isr_entry_count++;

	/* Check if data is available */
	uint8_t lsr = uart_read_reg(UART_LSR);
	while (lsr & UART_LSR_DR) {
		/* Read the character */
		uint8_t c = uart_read_reg(UART_RBR);
		rx_char_count++;

		/* Echo it back */
		uart_write_reg(UART_THR, c);

		/* Wait for transmit to complete */
		while (!(uart_read_reg(UART_LSR) & UART_LSR_THRE)) {
			/* Wait */
		}

		/* Check for more data */
		lsr = uart_read_reg(UART_LSR);
	}
}

/**
 * Configure AIA routing for UART interrupt
 */
static int configure_aia_routing(void)
{
	int ret;
	const struct device *aplic = riscv_aplic_get_dev();

	if (!aplic) {
		printk("ERROR: APLIC device not found\n");
		return -ENODEV;
	}

	printk("\n[AIA Configuration]\n");
	printk("  APLIC source: %d\n", UART_IRQ_NUM);
	printk("  Target EIID: %d\n", UART_EIID);
	printk("  Priority: %d\n", UART_IRQ_PRIORITY);

	/* Configure APLIC source mode (edge-triggered for UART) */
	printk("\n  1. Configuring APLIC source mode...\n");
	ret = riscv_aplic_msi_config_src(aplic, UART_IRQ_NUM, APLIC_SM_EDGE_RISE);
	if (ret < 0) {
		printk("     ERROR: Failed to configure source: %d\n", ret);
		return ret;
	}
	printk("     ✓ Source configured as edge-triggered\n");

	/* Route APLIC source to EIID via MSI */
	printk("  2. Routing APLIC source %d → hart:0 eiid:%d\n",
	       UART_IRQ_NUM, UART_EIID);
	ret = riscv_aplic_msi_route(aplic, UART_IRQ_NUM, 0, UART_EIID);
	if (ret < 0) {
		printk("     ERROR: Failed to route source: %d\n", ret);
		return ret;
	}
	printk("     ✓ MSI route configured\n");

	/* Enable APLIC source */
	printk("  3. Enabling APLIC source...\n");
	riscv_aplic_enable_source(UART_IRQ_NUM);
	printk("     ✓ Source enabled\n");

	return 0;
}

/**
 * Test AIA path with GENMSI
 */
static void test_aia_genmsi(void)
{
	int pre_isr = isr_entry_count;

	printk("\n[GENMSI Test]\n");
	printk("  Injecting software MSI to EIID %d...\n", UART_EIID);

	riscv_aplic_inject_genmsi(0, UART_EIID);
	k_msleep(10);

	if (isr_entry_count > pre_isr) {
		printk("  ✓ GENMSI successfully triggered ISR!\n");
		printk("    ISR entry count: %d -> %d\n", pre_isr, isr_entry_count);
	} else {
		printk("  ✗ GENMSI did not trigger ISR\n");
	}
}

int main(void)
{
	int ret;

	printk("\n");
	printk("╔════════════════════════════════════════════════╗\n");
	printk("║  UART Echo - AIA with Zephyr Integration     ║\n");
	printk("╚════════════════════════════════════════════════╝\n");
	printk("\n");

	/* Get APLIC device */
	const struct device *aplic = riscv_aplic_get_dev();
	if (!aplic) {
		printk("ERROR: APLIC device not found\n");
		return -1;
	}
	printk("✓ APLIC device ready\n");

	printk("\n");
	printk("═══════════════════════════════════════════════\n");
	printk("  Configuration from Device Tree\n");
	printk("═══════════════════════════════════════════════\n");
	printk("  UART base: 0x%08x\n", UART_BASE);
	printk("  IRQ number: %d (APLIC source)\n", UART_IRQ_NUM);
	printk("  IRQ priority: %d\n", UART_IRQ_PRIORITY);
	printk("  Target EIID: %d\n", UART_EIID);
	printk("\n");

	/* Step 1: Configure AIA interrupt routing */
	printk("═══════════════════════════════════════════════\n");
	printk("  Step 1: Configure AIA Routing\n");
	printk("═══════════════════════════════════════════════\n");
	ret = configure_aia_routing();
	if (ret < 0) {
		printk("\nERROR: AIA configuration failed\n");
		return ret;
	}

	/* Step 2: Connect EIID to our ISR using Zephyr's IRQ_CONNECT */
	printk("\n");
	printk("═══════════════════════════════════════════════\n");
	printk("  Step 2: Connect EIID to ISR\n");
	printk("═══════════════════════════════════════════════\n");
	printk("  Connecting EIID %d to uart_eiid_isr...\n", UART_EIID);
	IRQ_CONNECT(UART_EIID, UART_IRQ_PRIORITY, uart_eiid_isr, NULL, 0);
	irq_enable(UART_EIID);
	printk("  ✓ ISR connected and EIID enabled\n");

	/* Step 3: Enable UART RX interrupts at hardware level */
	printk("\n");
	printk("═══════════════════════════════════════════════\n");
	printk("  Step 3: Enable UART Hardware Interrupts\n");
	printk("═══════════════════════════════════════════════\n");
	printk("  Writing 0x%02x to UART IER (0x%08x + 0x%02x)\n",
	       UART_IER_RDI, UART_BASE, UART_IER);
	uart_write_reg(UART_IER, UART_IER_RDI);
	uint8_t ier_readback = uart_read_reg(UART_IER);
	printk("  IER readback: 0x%02x\n", ier_readback);
	printk("  ✓ UART RX interrupts enabled\n");

	/* Step 4: Test AIA path with GENMSI */
	printk("\n");
	printk("═══════════════════════════════════════════════\n");
	printk("  Step 4: Test AIA Path with GENMSI\n");
	printk("═══════════════════════════════════════════════\n");
	test_aia_genmsi();

	/* Show interrupt flow */
	printk("\n");
	printk("═══════════════════════════════════════════════\n");
	printk("  Interrupt Flow\n");
	printk("═══════════════════════════════════════════════\n");
	printk("\n");
	printk("  UART RX → APLIC Source %d → MSI Write →\n", UART_IRQ_NUM);
	printk("  IMSIC EIID %d → CPU MEXT → uart_eiid_isr()\n", UART_EIID);
	printk("\n");
	printk("═══════════════════════════════════════════════\n");
	printk("  Ready! Type characters to see them echoed.\n");
	printk("═══════════════════════════════════════════════\n");
	printk("\n");

	/* Main loop - periodic status updates */
	int last_rx_count = 0;
	int last_isr_count = 1; /* Start at 1 to account for GENMSI test */

	while (1) {
		k_msleep(1000);

		/* Show status if there's activity */
		if (rx_char_count != last_rx_count || isr_entry_count != last_isr_count) {
			printk("[Status] ISR entries: %d, RX chars: %d\n",
			       isr_entry_count, rx_char_count);

			last_rx_count = rx_char_count;
			last_isr_count = isr_entry_count;
		}
	}

	return 0;
}

// SPDX-License-Identifier: Apache-2.0

/* Unified AIA coordinator - wraps APLIC and IMSIC for Zephyr integration */

#include <zephyr/kernel.h>
#include <zephyr/drivers/interrupt_controller/riscv_aia.h>
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(intc_riscv_aia, CONFIG_LOG_DEFAULT_LEVEL);

void riscv_aia_irq_enable(uint32_t irq)
{
	/* Enable in APLIC (wire interrupt source) */
	riscv_aplic_enable_source(irq);

	/* Enable in IMSIC (MSI target on CURRENT CPU) */
	riscv_imsic_enable_eiid(irq);
}

void riscv_aia_irq_disable(uint32_t irq)
{
	/* Disable in APLIC */
	riscv_aplic_disable_source(irq);

	/* Disable in IMSIC (on CURRENT CPU) */
	riscv_imsic_disable_eiid(irq);
}

int riscv_aia_irq_is_enabled(uint32_t irq)
{
	/* Check IMSIC enable state on CURRENT CPU */
	return riscv_imsic_is_enabled(irq);
}

void riscv_aia_set_priority(uint32_t irq, uint32_t prio)
{
	/* APLIC-MSI mode has no per-source priority registers.
	 * Priority in AIA is handled via IMSIC EITHRESHOLD (global threshold)
	 * or implicit EIID ordering (lower EIID = higher priority).
	 */
	if (prio != 0) {
		LOG_WRN("AIA-MSI: per-IRQ priority not supported (EIID %u, prio %u ignored)",
			irq, prio);
	}
}

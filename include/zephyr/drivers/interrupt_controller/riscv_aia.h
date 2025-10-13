/* SPDX-License-Identifier: Apache-2.0 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_AIA_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_AIA_H_

#include <zephyr/device.h>
#include <zephyr/types.h>

/* Unified AIA API - wraps APLIC and IMSIC */
void riscv_aia_irq_enable(uint32_t irq);
void riscv_aia_irq_disable(uint32_t irq);
int riscv_aia_irq_is_enabled(uint32_t irq);
void riscv_aia_set_priority(uint32_t irq, uint32_t prio);

#endif

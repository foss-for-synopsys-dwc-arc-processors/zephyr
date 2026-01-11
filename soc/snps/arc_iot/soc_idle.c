/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Custom CPU idle implementation for ARC IoT SoC
 *
 * When the ARC IoT board (iotdk) enters sleep mode, some peripherals
 * (e.g., UART) will power off and the board cannot wake up through
 * peripheral interrupts. This custom implementation skips the sleep
 * instruction and just enables interrupts.
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

/**
 * @brief Put the CPU in low-power mode
 *
 * This function always exits with interrupts unlocked.
 *
 * For ARC IoT SoC, we skip actual sleep mode because when the board
 * enters sleep mode, some peripherals (like UART) lose power and
 * the board cannot wake up through peripheral interrupts.
 */
void arch_cpu_idle(void)
{
	sys_trace_idle();
	irq_unlock(0);
}

/**
 * @brief Put the CPU in low-power mode, entered with IRQs locked
 *
 * This function exits with interrupts restored to @a key.
 *
 * For ARC IoT SoC, we skip actual sleep mode because when the board
 * enters sleep mode, some peripherals (like UART) lose power and
 * the board cannot wake up through peripheral interrupts.
 *
 * @param key The interrupt lock key returned by irq_lock()
 */
void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	irq_unlock(key);
}

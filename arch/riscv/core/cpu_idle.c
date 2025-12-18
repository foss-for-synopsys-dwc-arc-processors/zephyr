/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

/* Weak hook for WFI entry - can be overridden by application for diagnostics */
void __weak z_idle_wfi_entry_hook(void)
{
	/* Default implementation does nothing */
}

/* Weak hook for WFI exit - can be overridden by application for diagnostics */
void __weak z_idle_wfi_exit_hook(void)
{
	/* Default implementation does nothing */
}

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
	sys_trace_idle();

	/* Call diagnostic hook before WFI */
	z_idle_wfi_entry_hook();

	__asm__ volatile("wfi");

	/* Call diagnostic hook after WFI returns */
	z_idle_wfi_exit_hook();

	sys_trace_idle_exit();
	irq_unlock(MSTATUS_IEN);
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	__asm__ volatile("wfi");
	sys_trace_idle_exit();
	irq_unlock(key);
}
#endif

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>

/* Global diagnostic counters */
volatile uint32_t timer_isr_count = 0;      /* Timer interrupt handler calls */
volatile uint32_t timer_isr_count_before_wfi = 0;      /* Timer interrupt handler calls */
volatile uint32_t idle_wfi_exit_count = 0; /* Idle task WFI exits */


/*
 * Hook function for timer ISR - increment counter
 * This is a weak function that gets called from the timer driver
 */
void z_timer_test_hook(void)
{
	timer_isr_count++;
}

/*
 * Hook function for idle task before WFI
 * This gets called when WFI is about to be executed in idle task
 */
void z_idle_wfi_entry_hook(void)
{
	timer_isr_count_before_wfi = timer_isr_count;
}

/*
 * Hook function for idle task after WFI
 * This gets called when WFI returns in idle task
 */
void z_idle_wfi_exit_hook(void)
{
	idle_wfi_exit_count++;
}


int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);

	printf("Timer ISR calls:     %u\n", timer_isr_count);
	printf("Timer ISR calls before WFI:     %u\n", timer_isr_count_before_wfi);
	printf("Idle WFI exits:      %u\n", idle_wfi_exit_count);

	k_msleep(50);

	/* Print final diagnostics */
	printf("Timer ISR calls:     %u\n", timer_isr_count);
	printf("Timer ISR count before WFI:     %u\n", timer_isr_count_before_wfi);
	printf("Idle WFI exits:      %u\n", idle_wfi_exit_count);

	while (1) {

	}
	return 0;
}

/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <kernel.h>
#include <ksched.h>
#include <errno.h>
#include <zephyr/types.h>
#include <init.h>
#include <toolchain.h>

#include <arch/arc/v2/secureshield/arc_secure.h>

uint32_t normal_irq_switch_request;

static struct k_thread *normal_container_thread;

static void _default_sjli_entry(void);
/*
 * sjli vector table must be in instruction space
 * \todo: how to let user to install customized sjli entry easily, e.g.
 *  through macros or with the help of compiler?
 */
const static uint32_t _sjli_vector_table[CONFIG_SJLI_TABLE_SIZE] = {
	[0] = (uint32_t)_arc_do_secure_call,
	[1 ... (CONFIG_SJLI_TABLE_SIZE - 1)] = (uint32_t)_default_sjli_entry,
};

/*
 * @brief default entry of sjli call
 *
 */
static void _default_sjli_entry(void)
{
	printk("default sjli entry\n");
}

/*
 * @brief initialization of sjli related functions
 *
 */
static void sjli_table_init(void)
{
	/* install SJLI table */
	z_arc_v2_aux_reg_write(_ARC_V2_NSC_TABLE_BASE, _sjli_vector_table);
	z_arc_v2_aux_reg_write(_ARC_V2_NSC_TABLE_TOP,
			       (_sjli_vector_table + CONFIG_SJLI_TABLE_SIZE));
}

/*
 * @brief the handler of secure software helper irq
 *
 *  see comments in arc_secureshield_init for details
 */
static void secure_soft_int_handler(void *unused)
{
	ARG_UNUSED(unused);

	/* this is for the case where normal world slept and is waken up by
	 * normal irq.
	 */
	if (z_is_thread_state_set(normal_container_thread, _THREAD_SUSPENDED)) {
		k_thread_resume(normal_container_thread);
	}
}

/*
 * @brief initialization of secureshield related functions.
 */
static int arc_secureshield_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	sjli_table_init();

	/*  here set up a software triggered interrupt to help raise secure
	 * world's thread switch request from normal world, e.g., from normal
	 * interrupt. This is useful for the following case
	 *  - secure int preempted normal int and raise a thread switch
	 *    request which only can be handled when all irq, including normal
	 *    irq returns. This interrupt will help to do this.
	 *  - idle case: when normal world goes idle, the container thread of
	 *    normal world will suspend itself. Then if a normal irq comes and
	 *    wants to wake up the normal world. This interrupt will be raised
	 *   and resume the container thread.
	 * This interrupt is a secure interrupt with lowest irq priority which
	 * will guarantee all other interrupts are handled before it.
	 */
	IRQ_CONNECT(CONFIG_SECURE_SOFT_IRQ, CONFIG_NUM_IRQ_PRIO_LEVELS - 1,
		    secure_soft_int_handler, NULL, 0);
	z_arc_v2_irq_unit_prio_set(CONFIG_SECURE_SOFT_IRQ,
				   (CONFIG_NUM_IRQ_PRIO_LEVELS - 1) |
				   _ARC_V2_IRQ_PRIORITY_SECURE);
	irq_enable(CONFIG_SECURE_SOFT_IRQ);

	/* disable nic bit to disable seti/clri and
	 * sleep/wevt in normal mode, use secure service to replace
	 */
	__asm__ volatile("sflag  0");

	return 0;
}

/*
 * @brief go to normal world from secure firmware
 */
FUNC_NORETURN void z_arch_go_to_normal(uint32_t entry)
{
/* record the container secure thread which will be used in
 * secure software irq.
 * \todo more information should be recorded in the future
 */
	normal_container_thread = k_current_get();
	arc_go_to_normal(entry);
	CODE_UNREACHABLE;
}

/*
 * @brief secure service for sleep instruction in normal world
 */
void arc_s_service_sleep(uint32_t arg)
{
	uint32_t prio_level = arg & 0xf;
	uint32_t key;

	if (prio_level >= ARC_N_IRQ_START_LEVEL &&
	    prio_level < CONFIG_NUM_IRQ_PRIO_LEVELS) {
		/* set valid irq prio level according to sleep
		 * instruction
		 */
		key = arch_irq_lock();
		if ((key & 0xf) >= ARC_N_IRQ_START_LEVEL) {
			key = ((key & 0x30) | prio_level);
		}
		arch_irq_unlock(key);
	}
	/* normal world runs in the context of secure thread, the sleep of
	 * normal world means suspending of current running secure thread.
	 * When normal interrupts want to wake up the normal wold, it needs
	 * notify the secure world by secure software irq to resume the
	 * suspended secure thread, then returns from this function to normal
	 * world.
	 */
	k_thread_suspend(k_current_get());
}

/*
 * @brief secure service to handle normal world's context
 * switch request
 */
uint32_t arc_s_service_n_switch(void)
{
	struct k_thread *current;

	/* only can be called in isr context */
	if (!z_arc_v2_irq_unit_is_in_isr()) {
		return 0;
	}

	current = k_current_get();
	/* not in the container thread of normal world
	 * record it and replay it when the container thread
	 * comes back to run.
	 */
	if (normal_container_thread != current) {
		if (z_is_thread_state_set(normal_container_thread,
					  _THREAD_SUSPENDED)) {
			/* raise normal world from sleep by the helper irq
			 * the normal world switch request will be handled
			 * at the scheduling point when returning to normal
			 * world by secure function call return
			 */
			z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_HINT,
					       CONFIG_SECURE_SOFT_IRQ);

		}
	}
	normal_irq_switch_request = 1;

	return 0;
}

SYS_INIT(arc_secureshield_init, PRE_KERNEL_2,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

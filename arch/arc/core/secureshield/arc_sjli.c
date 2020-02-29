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

u32_t normal_irq_switch_request;

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
 * @brief initialization of secureshield related functions.
 */
static int arc_secureshield_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	sjli_table_init();

	/* disable nic bit to disable seti/clri and
	 * sleep/wevt in normal mode, use secure service to replace
	 */
	__asm__ volatile("sflag  0");

	return 0;
}

/*
 * @brief go to normal world from secure firmware
 */
FUNC_NORETURN void z_arch_go_to_normal(u32_t entry)
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
void arc_s_service_sleep(u32_t arg)
{
	u32_t prio_level = arg & 0xf;
	u32_t key;

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
u32_t arc_s_service_n_switch(void)
{
	struct k_thread *current;

	/* only can be called in isr context */
	if (!z_arc_v2_irq_unit_is_in_isr()) {
		return 0;
	}

	normal_irq_switch_request = 1;

	return 0;
}

SYS_INIT(arc_secureshield_init, PRE_KERNEL_2,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

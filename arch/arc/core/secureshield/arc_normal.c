/*
 * Copyright (c) 2020 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <kernel.h>
#include <errno.h>
#include <zephyr/types.h>
#include <init.h>
#include <toolchain.h>

/*
 * @brief the handler of normal software helper irq
 *
 * do nothing, just for handling context switch request in
 * the epilogue of interrupt handling.
 */
static void normal_soft_int_handler(void *unused)
{
	ARG_UNUSED(unused);

}

/*
 * @brief normal firmware initialization
 *
 */
static int arc_normal_firmware_init(struct device *arg)
{
	ARG_UNUSED(arg);

	/* here set up a software triggered interrupt to help raise the
	 * delayed context switch request
	 */
	IRQ_CONNECT(CONFIG_NORMAL_SOFT_IRQ, CONFIG_NUM_IRQ_PRIO_LEVELS - 1,
		    normal_soft_int_handler, NULL, 0);

	irq_enable(CONFIG_NORMAL_SOFT_IRQ);

	return 0;
}

/*
 * @brief secure call wrapper
 *
 * Currently, the secure call in secure world can accept maximum 6
 * args. Through this wrapper, the caller saved regs will be saved
 * and restored by toolchain.
 */
u32_t z_arc_s_call_invoke6(u32_t arg1, u32_t arg2, u32_t arg3,
			   u32_t arg4, u32_t arg5, u32_t arg6,
			   u32_t call_id)
{
	register u32_t ret __asm__("r0") = arg1;
	register u32_t r1 __asm__("r1") = arg2;
	register u32_t r2 __asm__("r2") = arg3;
	register u32_t r3 __asm__("r3") = arg4;
	register u32_t r4 __asm__("r4") = arg5;
	register u32_t r5 __asm__("r5") = arg6;
	register u32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "push blink\n"
			 "sjli %[id]\n"
			 "pop blink\n"
			 : "=r"(ret)
			 : [id] "i" (SJLI_CALL_ARC_SECURE),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r4), "r" (r5), "r" (r6));

	/* in current design, normal context switch cannot happen in secure
	 * world which works like a scheduler lock, secure world needs to
	 * notify this context switch request through CONFIG_NORMAL_SOFT_IRQ
	 */

	return ret;
}

SYS_INIT(arc_normal_firmware_init, PRE_KERNEL_2,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

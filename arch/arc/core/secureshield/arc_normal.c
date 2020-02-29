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

	return ret;
}

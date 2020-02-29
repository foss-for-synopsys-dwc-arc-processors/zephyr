
/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_SJLI_H
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_SJLI_H

/* SJLI ID for system secure service */
#define SJLI_CALL_ARC_SECURE	0

#define ARC_S_CALL_AUX_READ		0
#define ARC_S_CALL_AUX_WRITE		1
#define ARC_S_CALL_CLRI			3
#define ARC_S_CALL_SETI			4
#define ARC_S_CALL_MPU			5
#define ARC_S_CALL_SLEEP		6
#define ARC_S_CALL_N_SWITCH		7
#define ARC_S_CALL_LIMIT		8





/* the start irq priorirty used by normal firmware */
#if CONFIG_NUM_IRQ_PRIO_LEVELS <= CONFIG_SECURE_NUM_IRQ_PRIO_LEVELS
#define ARC_N_IRQ_START_LEVEL ((CONFIG_NUM_IRQ_PRIO_LEVELS + 1) / 2)
#else
#define ARC_N_IRQ_START_LEVEL	CONFIG_SECURE_NUM_IRQ_PRIO_LEVELS
#endif

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stdbool.h>

#include <arch/arc/v2/aux_regs.h>

#ifdef __cplusplus
extern "C" {
#endif


#define arc_sjli(id)	\
		(__asm__ volatile("sjli %[sjli_id]\n" :: [sjli_id] "i" (id)))

#ifdef CONFIG_ARC_SECURE_FIRMWARE
typedef u32_t (*_arc_s_call_handler_t)(u32_t arg1, u32_t arg2, u32_t arg3,
				      u32_t arg4, u32_t arg5, u32_t arg6);

extern FUNC_NORETURN void z_arch_go_to_normal(u32_t entry);
extern void arc_go_to_normal(u32_t addr);
extern void _arc_do_secure_call(void);
extern const _arc_s_call_handler_t arc_s_call_table[ARC_S_CALL_LIMIT];

#endif


#ifdef CONFIG_ARC_NORMAL_FIRMWARE

extern u32_t z_arc_s_call_invoke6(u32_t arg1, u32_t arg2, u32_t arg3,
			   u32_t arg4, u32_t arg5, u32_t arg6,
			   u32_t call_id);


#endif /* CONFIG_ARC_NORMAL_FIRMWARE */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_SECURE_H */

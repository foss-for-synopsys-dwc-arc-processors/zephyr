/*
 * Copyright (c) 2018 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <swap_macros.h>

.macro clear_scratch_regs
	mov r1, 0
	mov r2, 0
	mov r3, 0
	mov r4, 0
	mov r5, 0
	mov r6, 0
	mov r7, 0
	mov r8, 0
	mov r9, 0
	mov r10, 0
	mov r11, 0
	mov r12, 0
.endm

.macro clear_callee_regs
	mov r25, 0
	mov r24, 0
	mov r23, 0
	mov r22, 0
	mov r21, 0
	mov r20, 0
	mov r19, 0
	mov r18, 0
	mov r17, 0
	mov r16, 0

	mov r15, 0
	mov r14, 0
	mov r13, 0
.endm

/* macro to return normal world from secure call */
.macro _return_to_normal
	j [blink]
.endm

GTEXT(arc_go_to_normal)
GTEXT(_arc_do_secure_call)
GDATA(arc_s_call_table)


SECTION_FUNC(TEXT, _arc_do_secure_call)
	/* r0-r5: arg1-arg6, r6 is call id */
	/* the call id should be checked */
	cmp r6, ARC_S_CALL_CLRI
	beq _s_clri
	cmp r6, ARC_S_CALL_SETI
	beq _s_seti
	cmp r6, ARC_S_CALL_N_SWITCH
	beq _s_check_n_switch
_arc_call_secure_call_handler:
	push_s blink
	mov	blink, arc_s_call_table
	ld.as r6, [blink, r6]

	jl	[r6]

	/*
	 * no need to clear callee regs, as they will be saved and restored
	 * automatically
	 */
	clear_scratch_regs

	mov r29, 0
	mov r30, 0


_arc_do_secure_call_exit:
	pop_s blink

	_return_to_normal

/* secure service for clri
 * TODO: set watch dog to prevent DOS attack
 */
_s_clri:
	clri r0
	_return_to_normal

/* secure service for seti
 */
_s_seti:
	/* process the arg */
	and r6, r0, 0x0f
	/* only update valid priority level */
	cmp r6, (CONFIG_NUM_IRQ_PRIO_LEVELS - 1)
	mov.gt r6, (CONFIG_NUM_IRQ_PRIO_LEVELS -1)

	and r0, r0, 0x30
	or r0, r0, r6
	seti r0

	mov r0, 0

	_return_to_normal

_s_check_n_switch:
	/* quick check and quick return */
	lr r0, [_ARC_V2_SEC_STAT]
	bbit1 r0, _ARC_V2_SEC_STAT_IRM_BIT, _n_no_switch
	mov r0, 1
	j [blink]
_n_no_switch:
	b _arc_call_secure_call_handler

/* go to normal world, r0 is the entry */
SECTION_FUNC(TEXT, arc_go_to_normal)
	clear_callee_regs
	clear_scratch_regs

	mov fp, 0
	mov r29, 0
	mov r30, 0
	mov blink, 0

	jl [r0]
	/* should not come here */
	kflag 1


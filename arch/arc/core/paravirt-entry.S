/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Reset handler
 *
 * Reset handler that prepares the system for running C code.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <swap_macros.h>
#include <zephyr/arch/arc/asm-compat/assembler.h>
#ifdef CONFIG_ARC_EARLY_SOC_INIT
  #include <soc_ctrl.h>
#endif

GDATA(z_interrupt_stacks)
GDATA(z_main_stack)
GDATA(_VectorTable)

/* use one of the available interrupt stacks during init */


#define INIT_STACK z_interrupt_stacks
#define INIT_STACK_SIZE CONFIG_ISR_STACK_SIZE

GTEXT(__reset)
GTEXT(__start)
GTEXT(_z_arc_paravirt_entry)

/**
 * @brief Reset vector
 *
 * Ran when the system comes out of reset. The processor is at supervisor level.
 *
 * Locking interrupts prevents anything from interrupting the CPU.
 *
 * When these steps are completed, jump to _PrepC(), which will finish setting
 * up the system for running C code.
 */

SECTION_SUBSEC_FUNC(TEXT,_reset_and__start,__reset)
SECTION_SUBSEC_FUNC(TEXT,_reset_and__start,__start)
	/* lock interrupts: will get unlocked when switch to main task
	 * also make sure the processor in the correct status
	 */
	mov_s r0, 0
	kflag r0

#ifdef CONFIG_ARC_SECURE_FIRMWARE
	sflag r0
#endif
	/* interrupt related init */
#ifndef CONFIG_ARC_NORMAL_FIRMWARE
	/* IRQ_ACT and IRQ_CTRL should be initialized and set in secure mode */
	sr r0, [_ARC_V2_AUX_IRQ_ACT]
	sr r0, [_ARC_V2_AUX_IRQ_CTRL]
#endif
	sr r0, [_ARC_V2_AUX_IRQ_HINT]

	/* set the vector table base early,
	 * so that exception vectors can be handled.
	 */
	MOVR r0, _VectorTable
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	sr r0, [_ARC_V2_IRQ_VECT_BASE_S]
#else
	SRR r0, [_ARC_V2_IRQ_VECT_BASE]
#endif

	lr r0, [_ARC_V2_STATUS32]
	bset r0, r0, _ARC_V2_STATUS32_DZ_BIT
	kflag r0

#if defined(CONFIG_USERSPACE)
	lr r0, [_ARC_V2_STATUS32]
	bset r0, r0, _ARC_V2_STATUS32_US_BIT
	kflag r0
#endif

#ifdef CONFIG_ARC_USE_UNALIGNED_MEM_ACCESS
	lr r0, [_ARC_V2_STATUS32]
	bset r0, r0, _ARC_V2_STATUS32_AD_BIT
	kflag r0
#endif

/* Invalidate icache */
	lr r0, [_ARC_V2_I_CACHE_BUILD]
	and.f r0, r0, 0xff
	bz.nd done_icache_invalidate

	mov_s r2, 0
	sr r2, [_ARC_V2_IC_IVIC]
	/* writing to IC_IVIC needs 3 NOPs */
	nop_s
	nop_s
	nop_s
done_icache_invalidate:

/* Invalidate dcache */
	lr r3, [_ARC_V2_D_CACHE_BUILD]
	and.f r3, r3, 0xff
	bz.nd done_dcache_invalidate

	mov_s r1, 1
	sr r1, [_ARC_V2_DC_IVDC]

done_dcache_invalidate:

#ifdef CONFIG_ARC_PARAVIRT
/* Allow user sleep  */
	lr r0, [_ARC_V2_STATUS32]
	bset r0, r0, _ARC_V2_STATUS32_US_BIT
	kflag r0
#endif

#ifndef CONFIG_ARC_PARAVIRT
	j _z_arc_paravirt_entry
#else


/*
 * In ARCv2, the U bit can only be set through exception return
 */

/* the following codes are used to switch from kernel mode
 * to user mode by fake exception, because U bit can only be set
 * by exception
 */
_arc_go_to_user_space:
	lr r0, [_ARC_V2_STATUS32]
	bset r0, r0, _ARC_V2_STATUS32_U_BIT

	mov_s r1, _z_arc_paravirt_entry

	sr r0, [_ARC_V2_ERSTATUS]
	sr r1, [_ARC_V2_ERET]

	/* fake exception return */
	lr r0, [_ARC_V2_STATUS32]
	bset r0, r0, _ARC_V2_STATUS32_AE_BIT
	kflag r0

/* when exception returns from kernel to user, sp and _ARC_V2_USER_SP
 * /_ARC_V2_SECU_SP will be switched
 */
/*
	sr r5, [_ARC_V2_USER_SP]

	mov_s sp, blink

	mov_s r0, 0

	clear_callee_regs

	clear_scratch_regs

	mov fp, 0
	mov r29, 0
	mov r30, 0
	mov blink, 0
*/
	rtie

#endif /* !CONFIG_ARC_PARAVIRT */


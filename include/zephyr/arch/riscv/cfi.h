/*
 * Copyright (c) 2025 NXP Semicondutors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_CFI_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_CFI_H_

#define ARCH_CFI_UNDEFINED_RETURN_ADDRESS() __asm__ volatile(".cfi_undefined ra")

#ifdef CONFIG_HW_SHADOW_STACK

#include <zephyr/sys/util.h>
#include <zephyr/types.h>

/**
 * @brief Shadow stack entry type for RISC-V (Zicfiss).
 * Shadow stack entries are pointer-sized on RISC-V.
 */
typedef uintptr_t arch_thread_hw_shadow_stack_t;

/**
 * @brief Calculate the aligned size of a RISC-V shadow stack buffer.
 *
 * Computes a percentage of the thread stack size, rounded up to alignment,
 * with a minimum floor.
 */
#define ARCH_THREAD_HW_SHADOW_STACK_SIZE(size_) \
	MAX(ROUND_UP((CONFIG_HW_SHADOW_STACK_PERCENTAGE_SIZE * (size_) / 100), \
		      CONFIG_RISCV_CFI_SHADOW_STACK_ALIGNMENT), \
	    CONFIG_HW_SHADOW_STACK_MIN_SIZE)

/**
 * @brief Declare (extern) a single shadow stack buffer defined elsewhere.
 */
#define ARCH_THREAD_HW_SHADOW_STACK_DECLARE(sym, size) \
	extern arch_thread_hw_shadow_stack_t \
		sym[(size) / sizeof(arch_thread_hw_shadow_stack_t)]

/**
 * @brief Declare (extern) an array of shadow stack buffers defined elsewhere.
 */
#define ARCH_THREAD_HW_SHADOW_STACK_ARRAY_DECLARE(sym, nmemb, size) \
	extern arch_thread_hw_shadow_stack_t \
		sym[nmemb][(size) / sizeof(arch_thread_hw_shadow_stack_t)]

/**
 * @brief Define a single shadow stack buffer in the .riscvshadowstack section.
 *
 * The buffer is zero-initialized; the kernel fills in the initial SSP token
 * at thread creation time (arch_new_thread / arch_thread_hw_shadow_stack_attach).
 */
#define ARCH_THREAD_HW_SHADOW_STACK_DEFINE(name, size) \
	arch_thread_hw_shadow_stack_t \
		__aligned(CONFIG_RISCV_CFI_SHADOW_STACK_ALIGNMENT) \
		Z_GENERIC_SECTION(.riscvshadowstack) \
		name[(size) / sizeof(arch_thread_hw_shadow_stack_t)]

/**
 * @brief Define an array of shadow stack buffers in the .riscvshadowstack section.
 */
#define ARCH_THREAD_HW_SHADOW_STACK_ARRAY_DEFINE(name, nmemb, size) \
	arch_thread_hw_shadow_stack_t \
		__aligned(CONFIG_RISCV_CFI_SHADOW_STACK_ALIGNMENT) \
		Z_GENERIC_SECTION(.riscvshadowstack.arr_ ##name) \
		name[MAX(nmemb, 1)][(size) / sizeof(arch_thread_hw_shadow_stack_t)]

/**
 * @brief Attach a shadow stack buffer to a thread.
 *
 * Records the buffer base, size, and initial shadow stack pointer into
 * the thread's arch data.  Called by the generic kernel via
 * k_thread_hw_shadow_stack_attach().
 *
 * @param thread    Thread to attach the shadow stack to.
 * @param stack     Pointer to the shadow stack buffer.
 * @param stack_size Size of the shadow stack buffer in bytes.
 * @return 0 on success, negative errno on failure.
 */
int arch_thread_hw_shadow_stack_attach(struct k_thread *thread,
				       arch_thread_hw_shadow_stack_t *stack,
				       size_t stack_size);

#endif /* CONFIG_HW_SHADOW_STACK */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CFI_H_ */


/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @brief test context and thread APIs
 *
 * @defgroup kernel_context_tests Context Tests
 *
 * @ingroup all_tests
 *
 * This module tests the following CPU and thread related routines:
 * k_thread_create(), k_yield(), k_is_in_isr(),
 * k_current_get(), k_cpu_idle(), k_cpu_atomic_idle(),
 * irq_lock(), irq_unlock(),
 * irq_offload(), irq_enable(), irq_disable(),
 * @{
 * @}
 */

#include <stdlib.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sys_clock.h>

#if defined(CONFIG_SOC_POSIX)
/* TIMER_TICK_IRQ <soc.h> header for certain platforms */
#include <soc.h>
#endif

#define THREAD_STACKSIZE    (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_STACKSIZE2   (384 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_PRIORITY     4

#define THREAD_SELF_CMD    0
#define EXEC_CTX_TYPE_CMD  1

#define UNKNOWN_COMMAND    -1
#define INVALID_BEHAVIOUR  -2

/*
 * Get the timer type dependent IRQ number. If timer type
 * is not defined in platform, generate an error
 */

#if defined(CONFIG_APIC_TSC_DEADLINE_TIMER)
#define TICK_IRQ z_loapic_irq_base() /* first LVT interrupt */
#elif defined(CONFIG_CPU_CORTEX_M)
/*
 * The Cortex-M use the SYSTICK exception for the system timer, which is
 * not considered an IRQ by the irq_enable/Disable APIs.
 */
#elif defined(CONFIG_SPARC)
#elif defined(CONFIG_MIPS)
#elif defined(CONFIG_ARCH_POSIX)
#if  defined(CONFIG_BOARD_NATIVE_POSIX)
#define TICK_IRQ TIMER_TICK_IRQ
#else
/*
 * Other POSIX arch boards will skip the irq_disable() and irq_enable() test
 * unless TICK_IRQ is defined here for them
 */
#endif /* defined(CONFIG_ARCH_POSIX) */
#else

extern const int32_t z_sys_timer_irq_for_test;
#define TICK_IRQ (z_sys_timer_irq_for_test)

#endif

/* Cortex-M1, Nios II, and RISCV without CONFIG_RISCV_HAS_CPU_IDLE
 * do have a power saving instruction, so k_cpu_idle() returns immediately
 */
#if !defined(CONFIG_CPU_CORTEX_M1) && !defined(CONFIG_NIOS2) && \
	(!defined(CONFIG_RISCV) || defined(CONFIG_RISCV_HAS_CPU_IDLE))
#define HAS_POWERSAVE_INSTRUCTION
#endif


static struct k_timer timer;
static struct k_sem reply_timeout;
struct k_fifo timeout_order_fifo;

/**
 *
 * @brief Initialize kernel objects
 *
 * This routine initializes the kernel objects used in this module's tests.
 *
 */
static void kernel_init_objects(void)
{
	k_sem_init(&reply_timeout, 0, UINT_MAX);
	k_timer_init(&timer, NULL, NULL);
	k_fifo_init(&timeout_order_fifo);
}


#if defined(HAS_POWERSAVE_INSTRUCTION)
#if defined(CONFIG_TICKLESS_KERNEL)
static struct k_timer idle_timer;

static void idle_timer_expiry_function(struct k_timer *timer_id)
{
	k_timer_stop(&idle_timer);
}

static void _test_kernel_cpu_idle(int atomic)
{
	unsigned int key;
	uint32_t dur = k_ms_to_ticks_ceil32(1000);

	/* Set up a time to trigger events to exit idle mode */
	k_timer_init(&idle_timer, idle_timer_expiry_function, NULL);

	k_usleep(1);
	k_timer_start(&idle_timer, K_TICKS(dur), K_NO_WAIT);
	key = irq_lock();
	if (atomic) {
		k_cpu_atomic_idle(key);
	} else {
		k_cpu_idle();
	}
}

#else /* CONFIG_TICKLESS_KERNEL */
static void _test_kernel_cpu_idle(int atomic)
{
	int tms, tms2;
	int i;

	/* Align to a "ms boundary". */
	tms = k_uptime_get_32();
	while (tms == k_uptime_get_32()) {
#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(50);
#endif
	}

	tms = k_uptime_get_32();
	for (i = 0; i < 5; i++) { /* Repeat the test five times */
		if (atomic) {
			unsigned int key = irq_lock();

			k_cpu_atomic_idle(key);
		} else {
			k_cpu_idle();
		}
		/* calculating milliseconds per tick*/
		tms += k_ticks_to_ms_floor64(1);
		tms2 = k_uptime_get_32();
		zassert_false(tms2 < tms, "Bad ms per tick value computed,"
			      "got %d which is less than %d\n",
			      tms2, tms);
	}
}
#endif /* CONFIG_TICKLESS_KERNEL */

/**
 * @brief Test cpu idle function
 *
 * @details
 * Test Objective:
 * - The kernel architecture provide an idle function to be run when the system
 *   has no work for the current CPU
 * - This routine tests the k_cpu_atomic_idle() routine
 *
 * Testing techniques
 * - Functional and black box testing
 * - Interface testing
 *
 * Prerequisite Condition:
 * - HAS_POWERSAVE_INSTRUCTION is set
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Record system time before cpu enters idle state
 * -# Enter cpu idle state by k_cpu_atomic_idle()
 * -# Record system time after cpu idle state is interrupted
 * -# Compare the two system time values.
 *
 * Expected Test Result:
 * - cpu enters idle state for a given time
 *
 * Pass/Fail criteria:
 * - Success if the cpu enters idle state, failure otherwise.
 *
 * Assumptions and Constraints
 * - N/A
 *
 * @see k_cpu_atomic_idle()
 * @ingroup kernel_context_tests
 */
ZTEST(context_cpu_idle, test_cpu_idle_atomic)
{
#if defined(CONFIG_ARM) || defined(CONFIG_ARM64)
	ztest_test_skip();
#else
	_test_kernel_cpu_idle(1);
#endif
}


#else /* HAS_POWERSAVE_INSTRUCTION */
ZTEST(context_cpu_idle, test_cpu_idle_atomic)
{
	ztest_test_skip();
}
#endif

static void *context_setup(void)
{
	kernel_init_objects();

	return NULL;
}

ZTEST_SUITE(context_cpu_idle, NULL, context_setup, NULL, NULL, NULL);

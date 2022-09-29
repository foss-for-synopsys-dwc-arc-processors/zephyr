/*
 * Copyright (c) 2022 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "test_common.h"

#ifndef CONFIG_DSP
#error Rebuild with the DSP config option enabled
#endif

#ifndef CONFIG_DSP_SHARING
#error Rebuild with the DSP_SHARING config option enabled
#endif

extern void test_load_store(void);
/* test_calculation is arch specific */
extern void test_calculation(void);

void test_main(void)
{
	/*
	 * Enable round robin scheduling to allow both the low priority complex
	 * computation and load/store tasks to execute. The high priority complex
	 * computation and load/store tasks will preempt the low priority tasks
	 * periodically.
	 */
	k_sched_time_slice_set(10, THREAD_LOW_PRIORITY);

	/* Run the testsuite */
	ztest_test_suite(dsp_sharing,
			 ztest_unit_test(test_load_store),
			 ztest_unit_test(test_calculation));
	ztest_run_test_suite(dsp_sharing);
}

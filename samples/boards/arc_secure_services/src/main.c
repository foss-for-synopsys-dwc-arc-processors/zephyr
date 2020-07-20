/*
 * Copyright (c) 2018 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <soc.h>

#include "arch/arc/v2/secureshield/arc_ss_audit_logging.h"
#if defined(CONFIG_SOC_NSIM_SEM)
#define NORMAL_FIRMWARE_ENTRY 0x40000
#elif defined(CONFIG_SOC_EMSK)
#define NORMAL_FIRMWARE_ENTRY 0x20000
#endif


#define STACKSIZE 1024
#define PRIORITY 7
#define SLEEPTIME 1000
static struct audit_record record={
	.id = 0xABCD0000, .size = sizeof(struct audit_record) - 4};

void threadA(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);
	for(uint8_t idx = 0; idx<36; idx++)
	{
		ss_audit_add_record(&record);
		record.id++;
	}

	printk("Go to normal application\n");

	z_arch_go_to_normal(*((uint32_t *)(NORMAL_FIRMWARE_ENTRY)));

	printk("should not come here\n");

}

K_THREAD_DEFINE(thread_a, STACKSIZE, threadA, NULL, NULL, NULL,
		PRIORITY, 0, 0);


void main(void)
{
	/* necessary configuration before go to normal */
	int32_t i = 0;

	while (1) {
		printk("I am the %s thread in secure world: %d\n",
				 __func__, i++);
		k_msleep(SLEEPTIME);
	}
}

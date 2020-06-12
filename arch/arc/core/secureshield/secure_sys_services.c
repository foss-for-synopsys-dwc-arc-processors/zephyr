/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <kernel.h>
#include <arch/cpu.h>
#include <zephyr/types.h>
#include <soc.h>
#include <toolchain.h>

#include <arch/arc/v2/secureshield/arc_secure.h>

/* IRQ_ACT.U bit can also be set by normal world */
#define IRQ_PRIO_MASK (0xffffffff << ARC_N_IRQ_START_LEVEL)
/*
 * @brief read secure auxiliary regs on behalf of normal mode
 *
 * @param aux_reg address of aux reg
 *
 * Some aux regs require secure privilege, this function implements
 * an secure service to access secure aux regs. Check should be done
 * to decide whether the access is valid.
 */
static int32_t arc_s_aux_read(uint32_t aux_reg)
{
	return -1;
}

/*
 * @brief write secure auxiliary regs on behalf of normal mode
 *
 * @param aux_reg address of aux reg
 * @param val, the val to write
 *
 * Some aux regs require secure privilege, this function implements
 * an secure service to access secure aux regs. Check should be done
 * to decide whether the access is valid.
 */
static int32_t arc_s_aux_write(uint32_t aux_reg, uint32_t val)
{
	if (aux_reg == _ARC_V2_AUX_IRQ_ACT) {
		/* 0 -> ARC_N_IRQ_START_LEVEL allocated to secure world
		 * left prio levels allocated to normal world
		 */
		val &= IRQ_PRIO_MASK;
		val |= (z_arc_v2_aux_reg_read(_ARC_V2_AUX_IRQ_ACT) &
		       (~IRQ_PRIO_MASK));
		z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_ACT, val);

		return  0;
	}

	return -1;
}

/* Secure MPU service */
extern u32_t arc_secure_service_mpu(u32_t arg1, u32_t arg2, u32_t arg3,
				    u32_t arg4, u32_t ops);
/* Secure sleep service */
extern void arc_s_service_sleep(u32_t arg);

/* Secure service to check normal world's switch request */
extern u32_t arc_s_service_n_switch(void);

/* Secure service of audit logging, allowing to log critical or security related events */
extern u32_t arc_s_service_audit_logging(u32_t arg1, u32_t arg2, u32_t arg3,
				    u32_t arg4, u32_t ops);

/*
 * \todo, how to add secure service easily
 */
const _arc_s_call_handler_t arc_s_call_table[ARC_S_CALL_LIMIT] = {
	[ARC_S_CALL_AUX_READ] = (_arc_s_call_handler_t)arc_s_aux_read,
	[ARC_S_CALL_AUX_WRITE] = (_arc_s_call_handler_t)arc_s_aux_write,
	[ARC_S_CALL_SLEEP] = (_arc_s_call_handler_t)arc_s_service_sleep,
	[ARC_S_CALL_MPU] = (_arc_s_call_handler_t)arc_secure_service_mpu,
	[ARC_S_CALL_N_SWITCH] = (_arc_s_call_handler_t)arc_s_service_n_switch,
	[ARC_S_CALL_AUDIT_LOGGING] = (_arc_s_call_handler_t)arc_s_service_audit_logging,
};

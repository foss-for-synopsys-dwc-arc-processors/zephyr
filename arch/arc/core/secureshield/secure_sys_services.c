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
		val |= z_arc_v2_aux_reg_read(_ARC_V2_AUX_IRQ_ACT);
		z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_ACT, val);
		return  0;
	}

	return -1;
}

/*
 * @brief allocate interrupt for normal world
 *
 * @param intno, the interrupt to be allocated to normal world
 *
 * By default, most interrupts are configured to be secure in initialization.
 * If normal world wants to use an interrupt, through this secure service to
 * apply one. Necessary check should be done to decide whether the apply is
 * valid
 */
static int32_t arc_s_irq_alloc(uint32_t intno)
{
	z_arc_v2_irq_uinit_secure_set(intno, 0);
	return 0;
}

/* Secure MPU service */
extern uint32_t arc_secure_service_mpu(uint32_t arg1, uint32_t arg2,
                    uint32_t arg3, uint32_t arg4, uint32_t ops);

/* Secure service to check normal world's switch request */
extern uint32_t arc_s_service_n_switch(void);


/*
 * \todo, how to add secure service easily
 */
const _arc_s_call_handler_t arc_s_call_table[ARC_S_CALL_LIMIT] = {
	[ARC_S_CALL_AUX_READ] = (_arc_s_call_handler_t)arc_s_aux_read,
	[ARC_S_CALL_AUX_WRITE] = (_arc_s_call_handler_t)arc_s_aux_write,
	[ARC_S_CALL_IRQ_ALLOC] = (_arc_s_call_handler_t)arc_s_irq_alloc,
    [ARC_S_CALL_MPU] = (_arc_s_call_handler_t)arc_secure_service_mpu,
	[ARC_S_CALL_N_SWITCH] = (_arc_s_call_handler_t)arc_s_service_n_switch,
};

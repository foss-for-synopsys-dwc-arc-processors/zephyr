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
static int32_t arc_ss_logging_handle(uint32_t aux_reg)
{
	return -1;
}

/*
 * \todo, how to add secure service easily
 */
const _arc_s_call_handler_t arc_ss_call_table[ARC_SS_CALL_LIMIT] = {
	[ARC_SS_CALL_LOGGING] = (_arc_s_call_handler_t)arc_ss_logging_handle,
};

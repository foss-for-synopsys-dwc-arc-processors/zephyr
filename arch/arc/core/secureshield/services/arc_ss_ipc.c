/*
 * Copyright (c) 2020 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device.h>
#include <init.h>
#include <kernel.h>

#include "arch/arc/v2/secureshield/arc_ss_ipc.h"

uint32_t arc_s_service_ipc(uint32_t arg1, uint32_t arg2, uint32_t ops)
{
	// ss_verify_caller();
	// switch(ops){
	// 	case OP1:
	// 	break;
	// }
	return 0;
}
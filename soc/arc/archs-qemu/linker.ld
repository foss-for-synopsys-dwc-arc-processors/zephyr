/*
 * Copyright (c) 2019 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <generated_dts_board.h>
#include <autoconf.h>


/* Instruction Closely Coupled Memory (ICCM) base address and size */
#define SRAM_START		DT_SRAM_BASE_ADDRESS
#define SRAM_SIZE	    DT_SRAM_SIZE



#include <arch/arc/v2/linker.ld>

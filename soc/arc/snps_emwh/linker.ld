/*
 * Copyright (c) 2019 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Synopsys EM Wuhan platform.
 */

#include <generated_dts_board.h>
#include <autoconf.h>

/*
 * PS RAM base address and size
 *
 * PS RAM includes the exception vector table at reset, which is at
 * the beginning of the region.
 */
#if defined(DT_PS_RAM_BASE_ADDRESS) && (DT_PS_RAM_SIZE > 0)
#define SRAM_START		DT_PS_RAM_BASE_ADDRESS
#define SRAM_SIZE		DT_PS_RAM_SIZE
#endif

/* Instruction Closely Coupled Memory (ICCM) base address and size */
#if defined(DT_ICCM_BASE_ADDRESS) && (DT_ICCM_SIZE > 0)
#define ICCM_START		DT_ICCM_BASE_ADDRESS
#define ICCM_SIZE		DT_ICCM_SIZE
#endif

/*
 * DCCM base address and size. DCCM is the data memory.
 */
/* Data Closely Coupled Memory (DCCM) base address and size */
#if defined(DT_DCCM_BASE_ADDRESS) && (DT_DCCM_SIZE > 0)
#define DCCM_START		DT_DCCM_BASE_ADDRESS
#define DCCM_SIZE		DT_DCCM_SIZE
#endif

#include <arch/arc/v2/linker.ld>

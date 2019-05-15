/*
 * Copyright (c) 2017 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <generated_dts_board.h>
#include <soc.h>
#include <arch/arc/v2/mpu/arc_mpu.h>
#include <linker/linker-defs.h>

static struct arc_mpu_region mpu_regions[] = {
	/* Region SRAM */
	MPU_REGION_ENTRY("ROM",
			 (u32_t)_image_rom_start,
			 (u32_t)_image_rom_size,
			 REGION_ROM_ATTR),

	MPU_REGION_ENTRY("RAM",
			(u32_t)_image_ram_start,
			256 * 1024,
			REGION_KERNEL_RAM_ATTR),
	/* Region Peripheral */
	MPU_REGION_ENTRY("PERIPHERAL",
			 DT_UART_NS16550_PORT_0_BASE_ADDR,
			 DT_UART_NS16550_PORT_0_SIZE,
			 REGION_KERNEL_RAM_ATTR),
};

struct arc_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};

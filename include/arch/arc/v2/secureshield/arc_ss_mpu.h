/*
 * Copyright (c) 2020 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_SS_MPU_H
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_SS_MPU_H

#include <arch/arc/v2/secureshield/arc_secure.h>

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

#define SS_MPU_OP_INIT_ENTRY    0
#define SS_MPU_OP_WRITE_ATTR    1
#define SS_MPU_OP_READ_ATTR     2
#define SS_MPU_OP_WRITE_START   3
#define SS_MPU_OP_READ_START    4
#define SS_MPU_OP_WRITE_END     5
#define SS_MPU_OP_READ_END      6
#define SS_MPU_OP_PROBE         7

#ifdef CONFIG_ARC_NORMAL_FIRMWARE

static inline void ss_region_init(u32_t index, u32_t region_addr, u32_t size,
				  u32_t region_attr)
{
	z_arc_s_call_invoke6(index, region_addr, size, region_attr,
			     SS_MPU_OP_INIT_ENTRY, 0, ARC_S_CALL_MPU);
}

static inline void ss_region_set_attr(u32_t index, u32_t attr)
{
	z_arc_s_call_invoke6(index, 0, 0, attr, SS_MPU_OP_WRITE_ATTR,
			     0, ARC_S_CALL_MPU);
}

static inline u32_t ss_region_get_attr(u32_t index)
{
	return z_arc_s_call_invoke6(index, 0, 0, 0, SS_MPU_OP_READ_ATTR,
				    0, ARC_S_CALL_MPU);
}

static inline u32_t ss_region_get_start(u32_t index)
{
	return z_arc_s_call_invoke6(index, 0, 0, 0, SS_MPU_OP_READ_START,
				    0, ARC_S_CALL_MPU);
}

static inline void ss_region_set_start(u32_t index, u32_t start)
{
	z_arc_s_call_invoke6(index, start, 0, 0, SS_MPU_OP_WRITE_START,
			     0, ARC_S_CALL_MPU);
}

static inline u32_t ss_region_get_end(u32_t index)
{
	return z_arc_s_call_invoke6(index, 0, 0, 0, SS_MPU_OP_READ_END,
				    0, ARC_S_CALL_MPU);
}

static inline void ss_region_set_end(u32_t index, u32_t end)
{
	z_arc_s_call_invoke6(index, end, 0, 0, SS_MPU_OP_WRITE_END,
			     0, ARC_S_CALL_MPU);
}

/**
 * This internal function probes the given addr's MPU index.if not
 * in MPU, returns error
 */
static inline int ss_mpu_probe(u32_t addr)
{
	return z_arc_s_call_invoke6(addr, 0, 0, 0, SS_MPU_OP_PROBE,
				    0, ARC_S_CALL_MPU);
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /*ZEPHYR_INCLUDE_ARCH_ARV_V2_SS_MPU_H */

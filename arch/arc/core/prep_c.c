/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 *
 *
 * Initialization of full C support: zero the .bss, copy the .data if XIP,
 * call z_cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <zephyr/types.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/arch/arc/v2/aux_regs.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>


/* XXX - keep for future use in full-featured cache APIs */
#if 0
/**
 * @brief Disable the i-cache if present
 *
 * For those ARC CPUs that have a i-cache present,
 * invalidate the i-cache and then disable it.
 */

static void disable_icache(void)
{
	unsigned int val;

	val = z_arc_v2_aux_reg_read(_ARC_V2_I_CACHE_BUILD);
	val &= 0xff; /* version field */
	if (val == 0) {
		return; /* skip if i-cache is not present */
	}
	z_arc_v2_aux_reg_write(_ARC_V2_IC_IVIC, 0);
	__builtin_arc_nop();
	z_arc_v2_aux_reg_write(_ARC_V2_IC_CTRL, 1);
}

/**
 * @brief Invalidate the data cache if present
 *
 * For those ARC CPUs that have a data cache present,
 * invalidate the data cache.
 */

static void invalidate_dcache(void)
{
	unsigned int val;

	val = z_arc_v2_aux_reg_read(_ARC_V2_D_CACHE_BUILD);
	val &= 0xff; /* version field */
	if (val == 0) {
		return; /* skip if d-cache is not present */
	}
	z_arc_v2_aux_reg_write(_ARC_V2_DC_IVDC, 1);
}
#endif

#define AUX_CLN_ADDR			0x640
#define AUX_CLN_DATA			0x641

static inline unsigned int arc_cln_read_reg(unsigned int reg)
{
	z_arc_v2_aux_reg_write(AUX_CLN_ADDR, reg);

	return z_arc_v2_aux_reg_read(AUX_CLN_DATA);
}

static inline void arc_cln_write_reg(unsigned int reg, unsigned int data)
{
	z_arc_v2_aux_reg_write(AUX_CLN_ADDR, reg);

	z_arc_v2_aux_reg_write(AUX_CLN_DATA, data);
}

#define ARC_CLN_CACHE_STATUS			209
#define ARC_CLN_CACHE_STATUS_BUSY		BIT(23)
#define ARC_CLN_CACHE_STATUS_DONE		BIT(24)
#define ARC_CLN_CACHE_STATUS_EN			BIT(27)

#define ARC_CLN_CACHE_CMD			208
#define ARC_CLN_CACHE_CMD_OP_REG_INV		0b1001
#define ARC_CLN_CACHE_CMD_INCR			BIT(4)


static void arc_cluster_scm_enable(void)
{
#ifdef CONFIG_ISA_ARCV3
	/* Disable SCM, just in case. */
	arc_cln_write_reg(ARC_CLN_CACHE_STATUS, 0);

	/* Invalidate SCM before enabling. */
	arc_cln_write_reg(ARC_CLN_CACHE_CMD, ARC_CLN_CACHE_CMD_OP_REG_INV |
			  ARC_CLN_CACHE_CMD_INCR);
	while (arc_cln_read_reg(ARC_CLN_CACHE_STATUS) &
	       ARC_CLN_CACHE_STATUS_BUSY)
		;

	arc_cln_write_reg(ARC_CLN_CACHE_STATUS, ARC_CLN_CACHE_STATUS_EN);
#endif /* CONFIG_ISA_ARCV3 */
}

extern FUNC_NORETURN void z_cstart(void);
/**
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 */

void _PrepC(void)
{
	arc_cluster_scm_enable();

	z_bss_zero();
	z_data_copy();
	z_cstart();
	CODE_UNREACHABLE;
}

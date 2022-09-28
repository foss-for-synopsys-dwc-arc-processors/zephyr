/**
 * @file
 * @brief common definitions for the DSP sharing test application
 */

/*
 * Copyright (c) 2022 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DSPCONTEXT_H
#define _DSPCONTEXT_H

/*
 * Each architecture must define the following structures (which may be empty):
 *      'struct dsp_volatile_register_set'
 *      'struct dsp_non_volatile_register_set'
 *
 * Each architecture must also define the following macros:
 *      SIZEOF_DSP_VOLATILE_REGISTER_SET
 *      SIZEOF_DSP_NON_VOLATILE_REGISTER_SET
 * Those macros are used as sizeof(<an empty structure>) is compiler specific;
 * that is, it may evaluate to a non-zero value.
 *
 * Each architecture shall also have custom implementations of:
 *     _load_all_dsp_registers()
 *     _load_then_store_all_dsp_registers()
 *     _store_all_dsp_registers()
 */

#if defined(CONFIG_ISA_ARCV2)

struct dsp_volatile_register_set {
#ifdef CONFIG_ARC_DSP_BFLY_SHARING
	uintptr_t dsp_bfly0;
	uintptr_t agu_ap0;
	uintptr_t agu_os0;
#endif
};

struct dsp_non_volatile_register_set {
	/* No non-volatile dsp registers */
};

#define SIZEOF_DSP_VOLATILE_REGISTER_SET sizeof(struct dsp_volatile_register_set)
#define SIZEOF_DSP_NON_VOLATILE_REGISTER_SET 0

#else

#error  "Architecture must provide the following definitions:\n"
"\t'struct dsp_volatile_registers'\n"
"\t'struct dsp_non_volatile_registers'\n"
"\t'SIZEOF_DSP_VOLATILE_REGISTER_SET'\n"
"\t'SIZEOF_DSP_NON_VOLATILE_REGISTER_SET'\n"
#endif /* CONFIG_ISA_ARCV2 */

/* the set of ALL dsp registers */

struct dsp_register_set {
	struct dsp_volatile_register_set dsp_volatile;
	struct dsp_non_volatile_register_set dsp_non_volatile;
};

#define SIZEOF_DSP_REGISTER_SET \
	(SIZEOF_DSP_VOLATILE_REGISTER_SET + SIZEOF_DSP_NON_VOLATILE_REGISTER_SET)

/*
 * The following constants define the initial byte value used by the background
 * task, and the thread when loading up the dsp registers.
 */

#define MAIN_DSP_REG_CHECK_BYTE ((unsigned char)0xe5)
#define FIBER_DSP_REG_CHECK_BYTE ((unsigned char)0xf9)

#endif /* _DSPCONTEXT_H */

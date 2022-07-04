/*
 * Copyright (c) 2022 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HAR_SMARTPHONE_MODEL_H_
#define _HAR_SMARTPHONE_MODEL_H_

#include "mli_types.h"

#include <stdint.h>

/* ------------------- Model interface ------------------ */
/* Input tensor. To be filled with input image by user before calling inference */
/* function (har_smartphone_net). */
#define IN_POINTS (128 * 9)
extern mli_tensor *const har_smartphone_net_input;

/* Output tensor for model. Will be filled with probabilities vector by model */
#define OUT_POINTS (6)
extern mli_tensor *const har_smartphone_net_output;

/**
 * Inference function
 *
 * Get input data from har_smartphone_net_input tensor (FX format), fed it to
 * the neural network, and writes results to har_smartphone_net_output tensor
 * (FX format). It is user responsibility to prepare input tensor correctly
 * before calling this function and get result from output tensor after function
 * finish
 *
 * params:
 * debug_ir_root -  Path to intermediate vectors prepared in IDX format
 * (hardcoded names).
 *                  Provides opportunity to analyse intermediate results in
 *                  terms of similarity with reference data. If path is
 *                  incorrect it outputts only profiling data If NULL is passed,
 *                  no messages will be printed in inference
 */

void har_smartphone_net(const char *debug_ir_root);

/**
 * Model initialization function
 *
 * Initialize module internal data. User must call this function before he can
 * use the inference function. Initialization can be done once during program
 * execution.
 */

mli_status har_smartphone_init(void);

/* ----------------- Model configuration ---------------- */

/* Use user-implemented LSTM as layer 3 */
/* If not defined - uses default mli lib lstm kernel */
/* #define CUSTOM_USER_LSTM_LAYER3 */

#define MODEL_SA_8     (8)
#define MODEL_FX_16    (16)
#define MODEL_FX_8W16D (816)

#if !defined(MODEL_BIT_DEPTH)
#define MODEL_BIT_DEPTH (MODEL_FX_16)
#endif

#if !defined(MODEL_BIT_DEPTH) ||                                               \
	(MODEL_BIT_DEPTH != MODEL_SA_8 && MODEL_BIT_DEPTH != MODEL_FX_16 &&    \
	 MODEL_BIT_DEPTH != MODEL_FX_8W16D)
#error "MODEL_BIT_DEPTH must be defined correctly!"
#endif

#if (MODEL_BIT_DEPTH == MODEL_SA_8)
#undef CUSTOM_USER_LSTM_LAYER3
typedef int8_t d_type;
#define D_FIELD pi8
#else
typedef int16_t d_type;
#define D_FIELD pi16
#endif

#endif /* _HAR_SMARTPHONE_MODEL_H_ */

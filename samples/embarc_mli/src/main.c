/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>
#include "mli_api.h"

#include "cifar10_ref_inout.h"
#include "cifar10_model.h"

#ifndef MAX
/** @def Common Maximum macro function (two values)*/
#define MAX(A,B) (((A) > (B))? (A): (B))
#endif

/** @struct error measurement metrics for two vectors  */
typedef struct ref_to_pred_output_t {
	float max_abs_err;          /**< maximum absolute error  */
	float pred_vec_length;      /**< Length of predicted vector: SQRT(sum_i(pred[i]^2))  */
	float ref_vec_length;       /**< Length of reference vector: SQRT(sum_i(ref[i]^2))  */
	float noise_vec_length;     /**< Length of reference vector: SQRT(sum_i((ref[i]-pred[i])^2))  */
	float quant_err_vec_length; /**< Length of quantized error vector: SQRT(sum_i((ref[i]-Quantized(ref[i]))^2))  */
	float ref_to_noise_snr;     /**< Signal-to-noise ratio 10*log_10((ref_vec_length+eps)/(noise_vec_length+eps))  [dB]*/
	float noise_to_quant_ratio; /**< Noise-to-quantization_err ratio (noise_vec_length)/(quant_err_vec_length+eps)  */
} ref_to_pred_output;

const unsigned char kSingleIn[IN_POINTS] = IN_IMG_12;
const float kSingleOutRef[OUT_POINTS] = OUT_PROB_12;

//========================================================================================
//
// Other internal functions and routines
//
//========================================================================================

//================================================================================
// Transform MLI FX tensor to float array
//=================================================================================
static mli_status mli_hlp_fx_tensor_to_float (const mli_tensor * src, float *dst, uint32_t dst_size) {
	uint32_t elem_num = mli_hlp_count_elem_num(src, 0);
	if (elem_num > dst_size)
		return MLI_STATUS_LENGTH_ERROR;
	if (elem_num == 0)
		return MLI_STATUS_BAD_TENSOR;

	const float scale_val = 1.0f / (float) (1u << (src->el_params.fx.frac_bits));
	if (src->el_type == MLI_EL_FX_16) {
		int16_t *src_arr = src->data;
		for (int idx = 0; idx < elem_num; idx++)
			dst[idx] = (float) (scale_val * src_arr[idx]);
	} else {
		int8_t *src_arr = src->data;
		for (int idx = 0; idx < elem_num; idx++)
			dst[idx] = (float) (scale_val * src_arr[idx]);
	}
	return MLI_STATUS_OK;
}

//========================================================================================
// Image pre-processing for CIFAR-10 net
//========================================================================================
static void cifar10_preprocessing(const void * image_, mli_tensor * net_input_) {
	const unsigned char * in = image_;
	d_type * const dst = (d_type * const)net_input_->data;

	// Copying data  to input tensor with subtraction of average.
	// Data shift may be required depending on tensor format
	if (net_input_->el_params.fx.frac_bits == 7) {
		for (int idx = 0; idx < IN_POINTS; idx++)
			dst[idx] = (d_type)((int)in[idx] - 128);
	} else if (net_input_->el_params.fx.frac_bits > 7) {
		int shift_left = net_input_->el_params.fx.frac_bits - 7;
		for (int idx = 0; idx < IN_POINTS; idx++)
			dst[idx] = (d_type)((int)in[idx] - 128) << shift_left;
	} else {
		int shift_right = 7 - net_input_->el_params.fx.frac_bits;
		for (int idx = 0; idx < IN_POINTS; idx++)
			dst[idx] = (d_type)((int)in[idx] - 128)  >> shift_right; // w/o rounding
	}
}

//================================================================================
// Measure various error metrics between two float vectors
//=================================================================================
static int measure_err_vfloat(const float * ref_vec, const float * pred_vec, const int len,
			ref_to_pred_output *out) {
	float ref_accum = 0.f;
	float pred_accum = 0.f;
	float noise_accum = 0.f;
	float max_err = -1.f;

	if (len <= 0) {
		return -1;
	}
	for (int i = 0; i < len; i++) {
		ref_accum += ref_vec[i] * ref_vec[i];
		pred_accum += pred_vec[i] * pred_vec[i];
		noise_accum += (ref_vec[i] - pred_vec[i]) * (ref_vec[i] - pred_vec[i]);
		max_err = MAX(fabsf(ref_vec[i] - pred_vec[i]), max_err);
	}

	const float eps = 0.000000000000000001f;
	out->max_abs_err = max_err;
	out->noise_to_quant_ratio = 1.0f;
	out->quant_err_vec_length = 1.0f;
	out->pred_vec_length = sqrtf(pred_accum);
	out->ref_vec_length = sqrtf(ref_accum);
	out->noise_vec_length = sqrtf(noise_accum);
	out->ref_to_noise_snr = 10.f * log10f((ref_accum + eps) / (noise_accum + eps));

	return 0;
}


extern char __embarc_mli_rom_start[];
extern char __embarc_mli_rom_end[];
extern char __embarc_mli_loadaddr_rom[];

extern char __embarc_mli_data_start[];
extern char __embarc_mli_data_end[];
extern char __embarc_mli_loadaddr_data[];

extern char __embarc_mli_zdata_start[];
extern char __embarc_mli_zdata_end[];
extern char __embarc_mli_loadaddr_zdata[];

extern char __embarc_mli_model_p2_start[];
extern char __embarc_mli_model_p2_end[];
extern char __embarc_mli_loadaddr_model_p2[];

extern char __embarc_mli_model_start[];
extern char __embarc_mli_model_end[];
extern char __embarc_mli_loadaddr_model[];

extern char __embarc_mli_bss_start[];
extern char __embarc_mli_bss_end[];

extern char __embarc_mli_xdata_start[];
extern char __embarc_mli_xdata_end[];

extern char __embarc_mli_ydata_start[];
extern char __embarc_mli_ydata_end[];


void embarc_mli_init(void)
{
	(void)memcpy(&__embarc_mli_rom_start, &__embarc_mli_loadaddr_rom,
		 __embarc_mli_rom_end - __embarc_mli_rom_start);

	(void)memcpy(&__embarc_mli_data_start, &__embarc_mli_loadaddr_data,
		 __embarc_mli_data_end - __embarc_mli_data_start);

	(void)memcpy(&__embarc_mli_zdata_start, &__embarc_mli_loadaddr_zdata,
		 __embarc_mli_zdata_end - __embarc_mli_zdata_start);

	(void)memcpy(&__embarc_mli_model_p2_start, &__embarc_mli_loadaddr_model_p2,
		 __embarc_mli_model_p2_end - __embarc_mli_model_p2_start);

	(void)memcpy(&__embarc_mli_model_start, &__embarc_mli_loadaddr_model,
		 __embarc_mli_model_end - __embarc_mli_model_start);

	(void)memset(__embarc_mli_bss_start, 0,
		 __embarc_mli_bss_end - __embarc_mli_bss_start);

	(void)memset(__embarc_mli_xdata_start, 0,
		 __embarc_mli_xdata_end - __embarc_mli_xdata_start);

	(void)memset(__embarc_mli_ydata_start, 0,
		 __embarc_mli_ydata_end - __embarc_mli_ydata_start);

}

void _init_ad(void)
{
	__asm__ volatile(
		"lr r0, [0xa]\n\t"
		"bset r0, r0, 19\n\t"
		"flag r0\n\t"
		::);
}


int main(void)
{
	embarc_mli_init();

	size_t output_elements = mli_hlp_count_elem_num(cifar10_cf_net_output, 0);
	float * pred_data = malloc(output_elements * sizeof(float));

	if (pred_data == NULL) {
		printf("ERROR: Can't allocate memory for output\n");
		return -1;
	}

	cifar10_preprocessing(kSingleIn, cifar10_cf_net_input);

	cifar10_cf_net();

// Check result
	if (MLI_STATUS_OK == mli_hlp_fx_tensor_to_float(cifar10_cf_net_output, pred_data, output_elements)) {
		ref_to_pred_output err;
		measure_err_vfloat(kSingleOutRef, pred_data, output_elements, &err);
		//printf("Result Quality: S/N=%f (%f db)\n", err.ref_vec_length / err.noise_vec_length, err.ref_to_noise_snr);
	} else {
		printf("ERROR: Can't transform out tensor to float\n");
	}
	free(pred_data);
	return 0;
}

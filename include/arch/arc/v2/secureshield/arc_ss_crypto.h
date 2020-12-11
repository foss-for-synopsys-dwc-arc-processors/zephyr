/*
 * Copyright (c) 2020 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_SS_CRYPTO_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_SS_CRYPTO_H_

#include <tinycrypt/aes.h>
#include <tinycrypt/ecc.h>
#include <tinycrypt/ctr_prng.h>
#include <tinycrypt/ccm_mode.h>

#define SS_TINYCRYPT_OP_AES_ENCRYPT  			0
#define SS_TINYCRYPT_OP_AES_DECRYPT  			1
#define SS_TINYCRYPT_OP_AES_CTR_CRYPT			2
#define SS_TINYCRYPT_OP_CTR_PRNG_INIT			3
#define SS_TINYCRYPT_OP_CTR_PRNG_RESEED			4
#define SS_TINYCRYPT_OP_CTR_PRNG_GEN			5
#define SS_TINYCRYPT_OP_CTR_PRNG_UNINST			6
#define SS_TINYCRYPT_OP_AES_CBC_ENCRYPT			7
#define SS_TINYCRYPT_OP_AES_CBC_DECRYPT			8
#define SS_TINYCRYPT_OP_AES_CCM_CONFIG			9
#define SS_TINYCRYPT_OP_AES_CCM_GEN_ENCRYPT		10
#define SS_TINYCRYPT_OP_AES_CCM_DECRYPT_VERF	11
#define SS_TINYCRYPT_OP_SHA256       			12
#define SS_TINYCRYPT_OP_HMAC         			13
#define SS_TINYCRYPT_OP_ECC_SIGN				14
#define SS_TINYCRYPT_OP_ECC_VERIFY				15
#define SS_TINYCRYPT_OP_ECC_MAKE_KEY			16
#define SS_TINYCRYPT_OP_ECC_SHARED_SECRET		17
//TODO: set RNG by call uECC_set_rng(), this is needed if platform does not own a PRNG


typedef struct ss_crypto_data {
    uint32_t size;      /*!< Size in bytes of the payload fields */
    uint8_t  *payload;  /*!< Flexible array member for payload */
} ss_crypto_data_t, *ss_crypto_data_ptr;

uint32_t ss_crypto_tc_aes_encrypt(const uint8_t *key, const uint8_t *in,
										uint8_t *out);
uint32_t ss_crypto_tc_aes_decrypt(const uint8_t *key, const uint8_t *in,
										uint8_t *out);
uint32_t ss_crypto_tc_aes_ctr_crypt(const uint8_t *key, ss_crypto_data_ptr in,
										uint8_t *ctr, ss_crypto_data_ptr out);
uint32_t ss_crypto_tc_ctr_prng_init(TCCtrPrng_t * const ctx,
			ss_crypto_data_ptr entropy, ss_crypto_data_ptr personalization);
uint32_t ss_crypto_tc_ctr_prng_reseed(TCCtrPrng_t * const ctx,
			ss_crypto_data_ptr entropy, ss_crypto_data_ptr additional_input);
uint32_t ss_crypto_tc_ctr_prng_generate(TCCtrPrng_t * const ctx,
			ss_crypto_data_ptr additional_input, ss_crypto_data_ptr out);
uint32_t ss_crypto_tc_ctr_prng_uninstantiate(TCCtrPrng_t * const ctx);
uint32_t ss_crypto_tc_aes_cbc_encrypt(const uint8_t *key, ss_crypto_data_ptr in,
									const uint8_t *iv, ss_crypto_data_ptr out);
uint32_t ss_crypto_tc_aes_cbc_decrypt(const uint8_t *key, ss_crypto_data_ptr in,
									const uint8_t *iv, ss_crypto_data_ptr out);
uint32_t ss_crypto_tc_aes_ccm_config(TCCcmMode_t c, const uint8_t *key,
						ss_crypto_data_ptr nonce, uint32_t mlen);
uint32_t ss_crypto_tc_aes_ccm_generation_encryption(ss_crypto_data_ptr out,
				ss_crypto_data_ptr associated_data, ss_crypto_data_ptr payload,
				TCCcmMode_t c);
uint32_t ss_crypto_tc_aes_ccm_decryption_verification(ss_crypto_data_ptr out,
				ss_crypto_data_ptr associated_data, ss_crypto_data_ptr payload,
				TCCcmMode_t c);
uint32_t ss_crypto_tc_sha256(ss_crypto_data_ptr data, uint8_t * digest);
uint32_t ss_crypto_tc_hmac(ss_crypto_data_ptr key, ss_crypto_data_ptr data,
										uint8_t * digest);
uint32_t ss_crypto_tc_ecc_sign(const uint8_t *p_private_key,
	ss_crypto_data_ptr message_hash, uint8_t *p_signature, uECC_Curve curve);
uint32_t ss_crypto_tc_ecc_verify(const uint8_t *p_public_key,
		ss_crypto_data_ptr message_hash, const uint8_t *p_signature,
		uECC_Curve curve);
uint32_t ss_crypto_tc_ecc_make_key(uint8_t *p_public_key,
					uint8_t *p_private_key, uECC_Curve curve);
uint32_t ss_crypto_tc_ecc_shared_secret(const uint8_t *p_public_key,
			const uint8_t *p_private_key, uint8_t *p_secret, uECC_Curve curve);
#endif
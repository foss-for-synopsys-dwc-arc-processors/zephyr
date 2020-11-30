/*
 * Copyright (c) 2020 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_SS_CRYPTO_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_SS_CRYPTO_H_

#include <tinycrypt/ecc.h>

#define SS_TINYCRYPT_OP_AES_ENCRYPT  		0
#define SS_TINYCRYPT_OP_AES_DECRYPT  		1
#define SS_TINYCRYPT_OP_AES_CBC_ENCRYPT		2
#define SS_TINYCRYPT_OP_AES_CBC_DECRYPT		3
#define SS_TINYCRYPT_OP_SHA256       		4
#define SS_TINYCRYPT_OP_HMAC         		5
#define SS_TINYCRYPT_OP_ECC_SIGN			6
#define SS_TINYCRYPT_OP_ECC_VERIFY			7
#define SS_TINYCRYPT_OP_ECC_MAKE_KEY		8
#define SS_TINYCRYPT_OP_ECC_SHARED_SECRET	9
//TODO: set RNG by call uECC_set_rng(), this is needed if platform does not own a PRNG


typedef struct ss_crypto_data {
    uint32_t size;      /*!< Size in bytes of the payload fields */
    uint8_t  *payload;  /*!< Flexible array member for payload */
} ss_crypto_data_t, *ss_crypto_data_ptr;

uint32_t ss_crypto_tc_aes_encrypt(const uint8_t *key, const uint8_t *in,
										uint8_t *out);
uint32_t ss_crypto_tc_aes_decrypt(const uint8_t *key, const uint8_t *in,
										uint8_t *out);
uint32_t ss_crypto_tc_aes_cbc_encrypt(const uint8_t *key, ss_crypto_data_ptr in,
											const uint8_t *iv, uint8_t *out);
uint32_t ss_crypto_tc_aes_cbc_decrypt(const uint8_t *key, ss_crypto_data_ptr in,
											const uint8_t *iv, uint8_t *out);
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
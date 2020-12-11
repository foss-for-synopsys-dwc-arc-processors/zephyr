/*
 * Copyright (c) 2020 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device.h>
#include <init.h>
#include <kernel.h>
#include <string.h>

#include "arch/arc/v2/secureshield/arc_ss_crypto.h"
#include <tinycrypt/constants.h>
#include <tinycrypt/aes.h>
#include <tinycrypt/ctr_mode.h>
#include <tinycrypt/ctr_prng.h>
#include <tinycrypt/cbc_mode.h>
#include <tinycrypt/ccm_mode.h>
#include <tinycrypt/sha256.h>
#include <tinycrypt/hmac.h>
#include <tinycrypt/ecc.h>
#include <tinycrypt/ecc_dsa.h>
#include <tinycrypt/ecc_dh.h>


#if defined(CONFIG_ARC_SECURE_FIRMWARE)

// #include <sys/util.h>
// #include <random/rand32.h>
//TODO: modify this to arc specific
int default_CSPRNG(uint8_t *dest, unsigned int size)
{
	/* This is not a CSPRNG, but it's the only thing available in the
	 * system at this point in time.  */

	// while (size) {
	// 	uint32_t len = size >= sizeof(uint32_t) ? sizeof(uint32_t) : size;
	// 	uint32_t rv = sys_rand32_get();

	// 	memcpy(dest, &rv, len);
	// 	dest += len;
	// 	size -= len;
	// }

	return 1;
}

uint32_t ss_crypto_tc_aes_encrypt(const uint8_t *key, const uint8_t *in,
										uint8_t *out)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	struct tc_aes_key_sched_struct s;
	//TODO: the key could be replaced by a key ID, leave the key to secure world could improve security
	//TODO: aes set encrypt key and aes encrypt functions should be two seperate api, but need to protect the key_sched_struct first
	ret = tc_aes128_set_encrypt_key(&s, key);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	ret = tc_aes_encrypt(out, in, &s);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	//TODO: ret value: TC_CRYPTO_SUCCESS=1, should have a unified success value for all secure servvices
	return ret;
}

uint32_t ss_crypto_tc_aes_decrypt(const uint8_t *key, const uint8_t *in,
										uint8_t *out)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	struct tc_aes_key_sched_struct s;
	ret = tc_aes128_set_decrypt_key(&s, key);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	ret = tc_aes_decrypt(out, in, &s);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	//TODO: ret value: TC_CRYPTO_SUCCESS=1, should have a unified success value for all secure servvices
	return ret;
}

uint32_t ss_crypto_tc_aes_ctr_crypt(const uint8_t *key, ss_crypto_data_ptr in,
										uint8_t *ctr, ss_crypto_data_ptr out)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	struct tc_aes_key_sched_struct s;
	ret = tc_aes128_set_encrypt_key(&s, key);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	ret = tc_ctr_mode(out->payload, out->size, in->payload, in->size, ctr, &s);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	//TODO: ret value: TC_CRYPTO_SUCCESS=1, should have a unified success value for all secure servvices
	return ret;

}

uint32_t ss_crypto_tc_ctr_prng_init(TCCtrPrng_t * const ctx,
			ss_crypto_data_ptr entropy, ss_crypto_data_ptr personalization)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	ret = tc_ctr_prng_init(ctx, entropy->payload, entropy->size,
					personalization->payload, personalization->size);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	return ret;
}

uint32_t ss_crypto_tc_ctr_prng_reseed(TCCtrPrng_t * const ctx,
			ss_crypto_data_ptr entropy, ss_crypto_data_ptr additional_input)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	ret = tc_ctr_prng_reseed(ctx, entropy->payload, entropy->size,
					additional_input->payload, additional_input->size);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	return ret;
}

uint32_t ss_crypto_tc_ctr_prng_generate(TCCtrPrng_t * const ctx,
			ss_crypto_data_ptr additional_input, ss_crypto_data_ptr out)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	ret = tc_ctr_prng_generate(ctx, additional_input->payload,
					additional_input->size, out->payload, out->size);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	return ret;
}

uint32_t ss_crypto_tc_ctr_prng_uninstantiate(TCCtrPrng_t * const ctx)
{
	tc_ctr_prng_uninstantiate(ctx);
	return TC_CRYPTO_SUCCESS;
}

uint32_t ss_crypto_tc_aes_cbc_encrypt(const uint8_t *key, ss_crypto_data_ptr in,
									const uint8_t *iv, ss_crypto_data_ptr out)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	struct tc_aes_key_sched_struct s;
	//TODO: the key could be replaced by a key ID, leave the key to secure world could improve security
	ret = tc_aes128_set_encrypt_key(&s, key);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	ret = tc_cbc_mode_encrypt(out->payload, out->size, in->payload,
									in->size, iv, &s);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	//TODO: ret value: TC_CRYPTO_SUCCESS=1, should have a unified success value for all secure servvices
	return ret;
}

uint32_t ss_crypto_tc_aes_cbc_decrypt(const uint8_t *key, ss_crypto_data_ptr in,
									const uint8_t *iv, ss_crypto_data_ptr out)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	struct tc_aes_key_sched_struct s;
	ret = tc_aes128_set_decrypt_key(&s, key);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	ret = tc_cbc_mode_decrypt(out->payload, out->size, in->payload, in->size,
										iv, &s);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	//TODO: ret value: TC_CRYPTO_SUCCESS=1, should have a unified success value for all secure servvices
	return ret;

}

uint32_t ss_crypto_tc_aes_ccm_config(TCCcmMode_t c, const uint8_t *key,
						ss_crypto_data_ptr nonce, uint32_t mlen)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	struct tc_aes_key_sched_struct s;
	ret = tc_aes128_set_decrypt_key(&s, key);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	ret = tc_ccm_config(c, &s, nonce->payload, nonce->size, mlen);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	return ret;
}

uint32_t ss_crypto_tc_aes_ccm_generation_encryption(ss_crypto_data_ptr out,
				ss_crypto_data_ptr associated_data, ss_crypto_data_ptr payload,
				TCCcmMode_t c)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	ret = tc_ccm_generation_encryption(out->payload, out->size,
							associated_data->payload, associated_data->size,
							payload->payload, payload->size, c);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	return ret;
}

uint32_t ss_crypto_tc_aes_ccm_decryption_verification(ss_crypto_data_ptr out,
				ss_crypto_data_ptr associated_data, ss_crypto_data_ptr payload,
				TCCcmMode_t c)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	ret = tc_ccm_decryption_verification(out->payload, out->size,
							associated_data->payload, associated_data->size,
							payload->payload, payload->size, c);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	return ret;
}

uint32_t ss_crypto_tc_sha256(ss_crypto_data_ptr data, uint8_t * digest)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	struct tc_sha256_state_struct s;

	ret = tc_sha256_init(&s);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	ret = tc_sha256_update(&s, data->payload, data->size);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	ret = tc_sha256_final(digest, &s);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	return ret;
}

uint32_t ss_crypto_tc_hmac(ss_crypto_data_ptr key, ss_crypto_data_ptr data,
	                           uint8_t * digest)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	struct tc_hmac_state_struct h;
	(void)memset(&h, 0x00, sizeof(h));
	ret = tc_hmac_set_key(&h, key->payload, key->size);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	ret = tc_hmac_init(&h);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	ret = tc_hmac_update(&h, data->payload, data->size);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	ret = tc_hmac_final(digest, TC_SHA256_DIGEST_SIZE, &h);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	return ret;
}

uint32_t ss_crypto_tc_ecc_sign(const uint8_t *p_private_key,
		ss_crypto_data_ptr message_hash, uint8_t *p_signature, uECC_Curve curve)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	ret = uECC_sign(p_private_key, message_hash->payload, message_hash->size,
	      			p_signature, curve);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	return ret;
}

uint32_t ss_crypto_tc_ecc_verify(const uint8_t *p_public_key,
		ss_crypto_data_ptr message_hash, const uint8_t *p_signature,
		uECC_Curve curve)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	ret = uECC_verify(p_public_key, message_hash->payload, message_hash->size,
	      			p_signature, curve);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	return ret;
}

uint32_t ss_crypto_tc_ecc_make_key(uint8_t *p_public_key,
					uint8_t *p_private_key, uECC_Curve curve)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	ret = uECC_make_key(p_public_key, p_private_key, curve);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	return ret;
}

uint32_t ss_crypto_tc_ecc_shared_secret(const uint8_t *p_public_key,
			const uint8_t *p_private_key, uint8_t *p_secret, uECC_Curve curve)
{
	uint32_t ret = TC_CRYPTO_FAIL;
	ret = uECC_shared_secret(p_public_key, p_private_key, p_secret, curve);
	if(ret != TC_CRYPTO_SUCCESS)
	{
		return ret;
	}
	return ret;
}


uint32_t arc_s_service_crypto(uint32_t arg1, uint32_t arg2, uint32_t arg3,
				                  uint32_t arg4, uint32_t ops)
{
	switch(ops){
		case SS_TINYCRYPT_OP_AES_ENCRYPT:
			return ss_crypto_tc_aes_encrypt((uint8_t *)arg1, (uint8_t *)arg2,
											(uint8_t *)arg3);

		case SS_TINYCRYPT_OP_AES_DECRYPT:
			return ss_crypto_tc_aes_decrypt((uint8_t *)arg1, (uint8_t *)arg2,
											(uint8_t *)arg3);

		case SS_TINYCRYPT_OP_AES_CTR_CRYPT:
			return ss_crypto_tc_aes_ctr_crypt((uint8_t *)arg1,
		(ss_crypto_data_ptr)arg2, (uint8_t *)arg3, (ss_crypto_data_ptr)arg4);

		case SS_TINYCRYPT_OP_CTR_PRNG_INIT:
			return ss_crypto_tc_ctr_prng_init((TCCtrPrng_t * const)arg1,
						(ss_crypto_data_ptr)arg2, (ss_crypto_data_ptr)arg3);

		case SS_TINYCRYPT_OP_CTR_PRNG_RESEED:
			return ss_crypto_tc_ctr_prng_reseed((TCCtrPrng_t * const)arg1,
						(ss_crypto_data_ptr)arg2, (ss_crypto_data_ptr)arg3);

		case SS_TINYCRYPT_OP_CTR_PRNG_GEN:
			return ss_crypto_tc_ctr_prng_generate((TCCtrPrng_t * const)arg1,
						(ss_crypto_data_ptr)arg2, (ss_crypto_data_ptr)arg3);

		case SS_TINYCRYPT_OP_CTR_PRNG_UNINST:
			return ss_crypto_tc_ctr_prng_uninstantiate(
												(TCCtrPrng_t * const)arg1);

		case SS_TINYCRYPT_OP_AES_CBC_ENCRYPT:
			return ss_crypto_tc_aes_cbc_encrypt((uint8_t *)arg1,
		(ss_crypto_data_ptr)arg2, (uint8_t *)arg3, (ss_crypto_data_ptr)arg4);

		case SS_TINYCRYPT_OP_AES_CBC_DECRYPT:
			return ss_crypto_tc_aes_cbc_decrypt((uint8_t *)arg1,
		(ss_crypto_data_ptr)arg2, (uint8_t *)arg3, (ss_crypto_data_ptr)arg4);

		case SS_TINYCRYPT_OP_AES_CCM_CONFIG:
			return ss_crypto_tc_aes_ccm_config((TCCcmMode_t)arg1,
				(uint8_t *)arg2, (ss_crypto_data_ptr)arg3, arg4);

		case SS_TINYCRYPT_OP_AES_CCM_GEN_ENCRYPT:
			return ss_crypto_tc_aes_ccm_generation_encryption(
				(ss_crypto_data_ptr)arg1, (ss_crypto_data_ptr)arg2,
				(ss_crypto_data_ptr)arg3, (TCCcmMode_t)arg4);

		case SS_TINYCRYPT_OP_AES_CCM_DECRYPT_VERF:
			return ss_crypto_tc_aes_ccm_decryption_verification(
				(ss_crypto_data_ptr)arg1, (ss_crypto_data_ptr)arg2,
				(ss_crypto_data_ptr)arg3, (TCCcmMode_t)arg4);

		case SS_TINYCRYPT_OP_SHA256:
			return ss_crypto_tc_sha256((ss_crypto_data_ptr)arg1,
				            (uint8_t *)arg2);

		case SS_TINYCRYPT_OP_HMAC:
			return ss_crypto_tc_hmac((ss_crypto_data_ptr)arg1,
				            (ss_crypto_data_ptr)arg2, (uint8_t *)arg3);

		case SS_TINYCRYPT_OP_ECC_SIGN:
			return ss_crypto_tc_ecc_sign((uint8_t *)arg1,
				(ss_crypto_data_ptr)arg2, (uint8_t *)arg3, (uECC_Curve)arg4);

		case SS_TINYCRYPT_OP_ECC_VERIFY:
			return ss_crypto_tc_ecc_verify((uint8_t *)arg1,
				(ss_crypto_data_ptr)arg2, (uint8_t *)arg3, (uECC_Curve)arg4);

		case SS_TINYCRYPT_OP_ECC_MAKE_KEY:
			return ss_crypto_tc_ecc_make_key((uint8_t *)arg1,
				(uint8_t *)arg2, (uECC_Curve)arg3);

		case SS_TINYCRYPT_OP_ECC_SHARED_SECRET:
			return ss_crypto_tc_ecc_shared_secret((uint8_t *)arg1,
				(uint8_t *)arg2, (uint8_t *)arg3, (uECC_Curve)arg4);

		default:
			return 0;
	}
	return 0;
}

#else /* CONFIG_ARC_NORMAL_FIRMWARE */

uint32_t ss_crypto_tc_aes_encrypt(const uint8_t *key, const uint8_t *in,
										uint8_t *out)
{
	return z_arc_s_call_invoke6((uint32_t)key, (uint32_t)in, (uint32_t)out,
					0, SS_TINYCRYPT_OP_AES_ENCRYPT, 0, ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_aes_decrypt(const uint8_t *key, const uint8_t *in,
										uint8_t *out)
{
	return z_arc_s_call_invoke6((uint32_t)key, (uint32_t)in, (uint32_t)out,
					0, SS_TINYCRYPT_OP_AES_DECRYPT, 0, ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_aes_ctr_crypt(const uint8_t *key, ss_crypto_data_ptr in,
										uint8_t *ctr, ss_crypto_data_ptr out)
{
	return z_arc_s_call_invoke6((uint32_t)key, (uint32_t)in, (uint32_t)ctr,
		(uint32_t)out, SS_TINYCRYPT_OP_AES_CTR_CRYPT, 0, ARC_S_CALL_CRYPTO);
}
uint32_t ss_crypto_tc_ctr_prng_init(TCCtrPrng_t * const ctx,
			ss_crypto_data_ptr entropy, ss_crypto_data_ptr personalization)
{
	return z_arc_s_call_invoke6((uint32_t)ctx, (uint32_t)entropy,
			(uint32_t)personalization, 0, SS_TINYCRYPT_OP_CTR_PRNG_INIT, 0,
			ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_ctr_prng_reseed(TCCtrPrng_t * const ctx,
			ss_crypto_data_ptr entropy, ss_crypto_data_ptr additional_input)
{
	return z_arc_s_call_invoke6((uint32_t)ctx, (uint32_t)entropy,
			(uint32_t)additional_input, 0, SS_TINYCRYPT_OP_CTR_PRNG_RESEED, 0,
			ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_ctr_prng_generate(TCCtrPrng_t * const ctx,
			ss_crypto_data_ptr additional_input, ss_crypto_data_ptr out)
{
	return z_arc_s_call_invoke6((uint32_t)ctx, (uint32_t)additional_input,
		(uint32_t)out, 0, SS_TINYCRYPT_OP_CTR_PRNG_GEN, 0, ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_ctr_prng_uninstantiate(TCCtrPrng_t * const ctx)
{
	return z_arc_s_call_invoke6((uint32_t)ctx, 0, 0, 0,
			SS_TINYCRYPT_OP_CTR_PRNG_UNINST, 0, ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_aes_cbc_encrypt(const uint8_t *key, ss_crypto_data_ptr in,
									const uint8_t *iv, ss_crypto_data_ptr out)
{
	return z_arc_s_call_invoke6((uint32_t)key, (uint32_t)in, (uint32_t)iv,
		(uint32_t)out, SS_TINYCRYPT_OP_AES_CBC_ENCRYPT, 0, ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_aes_cbc_decrypt(const uint8_t *key, ss_crypto_data_ptr in,
									const uint8_t *iv, ss_crypto_data_ptr out)
{
	return z_arc_s_call_invoke6((uint32_t)key, (uint32_t)in, (uint32_t)iv,
		(uint32_t)out, SS_TINYCRYPT_OP_AES_CBC_DECRYPT, 0, ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_aes_ccm_config(TCCcmMode_t c, const uint8_t *key,
						ss_crypto_data_ptr nonce, unsigned int mlen)
{
	return z_arc_s_call_invoke6((uint32_t)c, (uint32_t)key, (uint32_t)nonce,
				mlen, SS_TINYCRYPT_OP_AES_CCM_CONFIG, 0, ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_aes_ccm_generation_encryption(ss_crypto_data_ptr out,
				ss_crypto_data_ptr associated_data, ss_crypto_data_ptr payload,
				TCCcmMode_t c)
{
	return z_arc_s_call_invoke6((uint32_t)out, (uint32_t)associated_data,
				(uint32_t)payload, (uint32_t)c,
				SS_TINYCRYPT_OP_AES_CCM_GEN_ENCRYPT, 0, ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_aes_ccm_decryption_verification(ss_crypto_data_ptr out,
				ss_crypto_data_ptr associated_data, ss_crypto_data_ptr payload,
				TCCcmMode_t c)
{
	return z_arc_s_call_invoke6((uint32_t)out, (uint32_t)associated_data,
				(uint32_t)payload, (uint32_t)c,
				SS_TINYCRYPT_OP_AES_CCM_DECRYPT_VERF, 0, ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_sha256(ss_crypto_data_ptr data, uint8_t * digest)
{
	return z_arc_s_call_invoke6((uint32_t)data, (uint32_t)digest, 0, 0,
				    SS_TINYCRYPT_OP_SHA256, 0, ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_hmac(ss_crypto_data_ptr key, ss_crypto_data_ptr data,
	                           uint8_t * digest)
{
	return z_arc_s_call_invoke6((uint32_t)key, (uint32_t)data, (uint32_t)digest,
					0, SS_TINYCRYPT_OP_HMAC, 0, ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_ecc_sign(const uint8_t *p_private_key,
		ss_crypto_data_ptr message_hash, uint8_t *p_signature, uECC_Curve curve)
{
	return z_arc_s_call_invoke6((uint32_t)p_private_key, (uint32_t)message_hash,
		(uint32_t)p_signature, (uint32_t)curve, SS_TINYCRYPT_OP_ECC_SIGN, 0,
		ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_ecc_verify(const uint8_t *p_public_key,
		ss_crypto_data_ptr message_hash, const uint8_t *p_signature,
		uECC_Curve curve)
{
	return z_arc_s_call_invoke6((uint32_t)p_public_key, (uint32_t)message_hash,
		(uint32_t)p_signature, (uint32_t)curve, SS_TINYCRYPT_OP_ECC_VERIFY, 0,
		ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_ecc_make_key(uint8_t *p_public_key,
					uint8_t *p_private_key, uECC_Curve curve)
{
	return z_arc_s_call_invoke6((uint32_t)p_public_key, (uint32_t)p_private_key,
		(uint32_t)curve, 0, SS_TINYCRYPT_OP_ECC_MAKE_KEY, 0, ARC_S_CALL_CRYPTO);
}

uint32_t ss_crypto_tc_ecc_shared_secret(const uint8_t *p_public_key,
			const uint8_t *p_private_key, uint8_t *p_secret, uECC_Curve curve)
{
	return z_arc_s_call_invoke6((uint32_t)p_public_key, (uint32_t)p_private_key,
		(uint32_t)p_secret, (uint32_t)curve, SS_TINYCRYPT_OP_ECC_SHARED_SECRET,
		0, ARC_S_CALL_CRYPTO);
}

#endif /* CONFIG_ARC_SECURE_FIRMWARE */
/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Include Standard C Library
 * ============================================================================
 */

/*
 * Self
 */
#include "pse_oob_sec_base.h"
#include "pse_oob_sec_internal.h"

#include <string.h>
#include <stdio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(OOB_SEC_UNITTEST, CONFIG_OOB_LOGGING);

/*
 * Include mbedTLS Library
 * ============================================================================
 */

#include "mbedtls/sha256.h"     /* SHA-256 only */
#include "mbedtls/md.h"         /* generic interface */
#include "mbedtls/gcm.h"        /* mbedtls_gcm_context */


#if (defined(SEC_UNIT_TEST) && SEC_UNIT_TEST)

/*
 * Global Variable for Unit Testing
 * ============================================================================
 */

void *unit_test_param[SEC_UNIT_TEST_PARAM_SIZE];
unsigned int clear_text_size_for_hash;
unsigned int default_ctx_init_value_in_byte;
unsigned int mbedtls_hardware_poll_flag;

/* See https://github.com/wolfeidau/mbedtls/tree/master/test/example-hashing
 *  for test pattern.
 */
SEC_RTN int unit_test_sec_int_verify_context_hash(void)
{
	int result;
	unsigned char *clear_text;


	unsigned char expected_hash[] = {
		0x31, 0x5f, 0x5b, 0xdb, 0x76, 0xd0, 0x78, 0xc4, 0x3b, 0x8a,
		0xc0, 0x06, 0x4e, 0x4a, 0x01, 0x64, 0x61, 0x2b, 0x1f, 0xce,
		0x77, 0xc8, 0x69, 0x34, 0x5b, 0xfc, 0x94, 0xc7, 0x58, 0x94,
		0xed, 0xd3
	};

	result = SEC_SUCCESS;

	/* Initialize clear text value */
	clear_text = (unsigned char *)&(sec_ctx.sec_ctx);
	clear_text_size_for_hash = 13;

	clear_text[0] = 'H';
	clear_text[1] = 'e';
	clear_text[2] = 'l';
	clear_text[3] = 'l';
	clear_text[4] = 'o';
	clear_text[5] = ',';
	clear_text[6] = ' ';
	clear_text[7] = 'w';
	clear_text[8] = 'o';
	clear_text[9] = 'r';
	clear_text[10] = 'l';
	clear_text[11] = 'd';
	clear_text[12] = '!';

	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	/* Initialize hash value */
	for (int i = 0; i < SEC_HASH_LEN; i++) {
		sec_ctx.a_hash[i] = expected_hash[i];
	}

	SEC_ASSERT(((sec_int_verify_context_hash()) == SEC_SUCCESS));

	return result;
}

/* See https://github.com/wolfeidau/mbedtls/tree/master/test/example-hashing
 *  for test pattern.
 */
SEC_RTN int unit_test_sec_int_update_context_hash(void)
{
	int result;
	unsigned char *clear_text;
	unsigned int clear_text_size_for_hash;

	unsigned char expected_hash[] = {
		0x31, 0x5f, 0x5b, 0xdb, 0x76, 0xd0, 0x78, 0xc4, 0x3b, 0x8a,
		0xc0, 0x06, 0x4e, 0x4a, 0x01, 0x64, 0x61, 0x2b, 0x1f, 0xce,
		0x77, 0xc8, 0x69, 0x34, 0x5b, 0xfc, 0x94, 0xc7, 0x58, 0x94,
		0xed, 0xd3
	};

	result = SEC_SUCCESS;

	/* Initialize clear text value */
	clear_text = (unsigned char *)&(sec_ctx.sec_ctx);
	clear_text_size_for_hash = 13;

	clear_text[0] = 'H';
	clear_text[1] = 'e';
	clear_text[2] = 'l';
	clear_text[3] = 'l';
	clear_text[4] = 'o';
	clear_text[5] = ',';
	clear_text[6] = ' ';
	clear_text[7] = 'w';
	clear_text[8] = 'o';
	clear_text[9] = 'r';
	clear_text[10] = 'l';
	clear_text[11] = 'd';
	clear_text[12] = '!';

	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	for (int i = 0; i < SEC_HASH_LEN; i++) {

		SEC_ASSERT((expected_hash[i] == sec_ctx.a_hash[i]));
	}

	return result;
}

SEC_RTN int unit_test_sec_int_init_context(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char expected_hash_0[] = {
		0x93, 0x76, 0x78, 0x38, 0x51, 0x7f, 0xe3, 0xc2, 0xe7, 0xca,
		0x68, 0xd1, 0xe7, 0x26, 0x47, 0x0b, 0xdd, 0xea, 0x4a, 0xc3,
		0xfd, 0x40, 0xce, 0xf4, 0xa5, 0xfb, 0x80, 0x76, 0x99, 0x32,
		0x6d, 0x0d
	};

	unsigned char expected_hash_1[] = {
		0x67, 0x6e, 0x82, 0xf8, 0x0f, 0x51, 0x46, 0x89, 0x22, 0xd4,
		0xbd, 0xd3, 0x4b, 0x53, 0xab, 0xd8, 0x68, 0x58, 0x55, 0xbc,
		0x01, 0x7a, 0x58, 0xfc, 0x26, 0xe9, 0x3b, 0xd0, 0xfd, 0x70,
		0x03, 0x17
	};

	result = SEC_SUCCESS;

	unit_test_param[0] = (void *)&clear_text_size_for_hash;
	unit_test_param[1] = (void *)&default_ctx_init_value_in_byte;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);

	default_ctx_init_value_in_byte = 97;
	/* 7436 of ASCII 'a', cross check with
	 * https://www.movable-type.co.uk/scripts/sha256.html
	 */

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_HASH_LEN; i++) {
		SEC_ASSERT((expected_hash_0[i] == sec_ctx.a_hash[i]));
	}

	default_ctx_init_value_in_byte = 0;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_HASH_LEN; i++) {
		SEC_ASSERT((expected_hash_1[i] == sec_ctx.a_hash[i]));
	}

	return result;
}

SEC_RTN int unit_test_sec_int_get_context_param(void)
{
	int result;

#define IN_BUF_SIZE 10
#define OUT_BUF_SIZE 20

	unsigned char a_sec_data[OUT_BUF_SIZE] = "aaaaaaaaaabbbbbbbbbb";
	unsigned int sec_len = OUT_BUF_SIZE;
	unsigned char a_ctx_param_data[IN_BUF_SIZE] = "1234567890";
	unsigned int ctx_param_len = IN_BUF_SIZE;

	unsigned char a_expected_data[OUT_BUF_SIZE] = "1234567890bbbbbbbbbb";

	result = SEC_SUCCESS;

	SEC_ASSERT((sec_int_get_context_param(
			    a_sec_data,
			    &sec_len,
			    a_ctx_param_data,
			    &ctx_param_len
			    ) == SEC_SUCCESS));

	for (int i = 0; i < OUT_BUF_SIZE; i++) {
		SEC_ASSERT((a_expected_data[i] == a_sec_data[i]));
	}

	SEC_ASSERT((sec_len == IN_BUF_SIZE));

	return result;
}

SEC_RTN int unit_test_sec_int_set_context_param(void)
{
	int result;

#define IN_BUF_SIZE 10
#define OUT_BUF_SIZE 20

	unsigned char a_sec_data[IN_BUF_SIZE] = "1234567890";
	unsigned int sec_len = IN_BUF_SIZE;
	unsigned char a_ctx_param_data[OUT_BUF_SIZE] = "bbbbbbbbbbaaaaaaaaaa";
	unsigned int ctx_param_len = OUT_BUF_SIZE;

	unsigned char a_expected_data[OUT_BUF_SIZE] = "1234567890aaaaaaaaaa";

	result = SEC_SUCCESS;

	SEC_ASSERT((sec_int_set_context_param(
			    a_sec_data,
			    &sec_len,
			    a_ctx_param_data,
			    &ctx_param_len
			    ) == SEC_SUCCESS));

	for (int i = 0; i < OUT_BUF_SIZE; i++) {
		SEC_ASSERT((a_expected_data[i] == a_ctx_param_data[i]));
	}

	SEC_ASSERT((ctx_param_len == IN_BUF_SIZE));

	return result;
}

SEC_RTN int unit_test_sec_int_get_context_param_simple(void)
{
	int result;

	unsigned int sec_data;
	unsigned int ctx_param_data;

	result = SEC_SUCCESS;

	/* Test Case 0: */

	sec_data = 0xaaaaaaaa;
	ctx_param_data = 0xffffffff;

	SEC_ASSERT((sec_int_get_context_param_simple(
			    &sec_data,
			    &ctx_param_data
			    ) == SEC_SUCCESS));

	SEC_ASSERT((sec_data == 0xffffffff));

	return result;
}

SEC_RTN int unit_test_sec_int_set_context_param_simple(void)
{
	int result;

	unsigned int sec_data;
	unsigned int ctx_param_data;

	result = SEC_SUCCESS;

	/* Test Case 0: */

	sec_data = 0xaaaaaaaa;
	ctx_param_data = 0xffffffff;

	SEC_ASSERT((sec_int_set_context_param_simple(
			    &sec_data,
			    &ctx_param_data
			    ) == SEC_SUCCESS));

	SEC_ASSERT((ctx_param_data == 0xaaaaaaaa));

	return result;
}

SEC_RTN int unit_test_sec_int_get_root_ca(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_ROOT_CA_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_ROOT_CA_LEN; i++) {
		sec_ctx.sec_ctx.a_root_ca[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.root_ca_len = SEC_ROOT_CA_LEN;
	sec_len = SEC_ROOT_CA_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_get_root_ca(
			    a_sec_data,
			    &sec_len
			    ) == SEC_SUCCESS));

	for (int i = 0; i < SEC_ROOT_CA_LEN; i++) {
		SEC_ASSERT(a_sec_data[i] == 0xff);
	}

	SEC_ASSERT(sec_len == SEC_ROOT_CA_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_set_root_ca(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_ROOT_CA_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_ROOT_CA_LEN; i++) {
		sec_ctx.sec_ctx.a_root_ca[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.root_ca_len = SEC_ROOT_CA_LEN;
	sec_len = SEC_ROOT_CA_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_set_root_ca(
			    a_sec_data,
			    &sec_len
			    ) == SEC_SUCCESS));

	for (int i = 0; i < SEC_ROOT_CA_LEN; i++) {
		SEC_ASSERT(sec_ctx.sec_ctx.a_root_ca[i] == 0xaa);
	}

	SEC_ASSERT(sec_ctx.sec_ctx.root_ca_len == SEC_ROOT_CA_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_get_owner_public_key(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_OWN_PUB_KEY_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_OWN_PUB_KEY_LEN; i++) {
		sec_ctx.sec_ctx.a_owner_public_key[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.owner_public_key_len = SEC_OWN_PUB_KEY_LEN;
	sec_len = SEC_OWN_PUB_KEY_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_get_owner_public_key(
			    a_sec_data,
			    &sec_len
			    ) == SEC_SUCCESS));

	for (int i = 0; i < SEC_OWN_PUB_KEY_LEN; i++) {
		SEC_ASSERT(a_sec_data[i] == 0xff);
	}

	SEC_ASSERT(sec_len == SEC_OWN_PUB_KEY_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_set_owner_public_key(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_OWN_PUB_KEY_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() ==
		    SEC_SUCCESS));

	for (int i = 0; i < SEC_OWN_PUB_KEY_LEN; i++) {
		sec_ctx.sec_ctx.a_owner_public_key[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.owner_public_key_len = SEC_OWN_PUB_KEY_LEN;
	sec_len = SEC_OWN_PUB_KEY_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_set_owner_public_key(
			    a_sec_data,
			    &sec_len
			    ) == SEC_SUCCESS));

	for (int i = 0; i < SEC_OWN_PUB_KEY_LEN; i++) {
		SEC_ASSERT(sec_ctx.sec_ctx.a_owner_public_key[i] == 0xaa);
	}

	SEC_ASSERT(
		sec_ctx.sec_ctx.owner_public_key_len == SEC_OWN_PUB_KEY_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_get_hkdf_32b_tpm_salt(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_TPM_SALT_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_TPM_SALT_LEN; i++) {
		sec_ctx.sec_ctx.a_hkdf_32b_tpm_salt[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.hkdf_32b_tpm_salt_len = SEC_TPM_SALT_LEN;
	sec_len = SEC_TPM_SALT_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_get_hkdf_32b_tpm_salt(a_sec_data,
						  &sec_len) == SEC_SUCCESS));

	for (int i = 0; i < SEC_TPM_SALT_LEN; i++) {
		SEC_ASSERT(a_sec_data[i] == 0xff);
	}

	SEC_ASSERT(sec_len == SEC_TPM_SALT_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_set_hkdf_32b_tpm_salt(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_TPM_SALT_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_TPM_SALT_LEN; i++) {
		sec_ctx.sec_ctx.a_hkdf_32b_tpm_salt[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.hkdf_32b_tpm_salt_len = SEC_TPM_SALT_LEN;
	sec_len = SEC_TPM_SALT_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_set_hkdf_32b_tpm_salt(
			    a_sec_data, &sec_len) == SEC_SUCCESS));

	for (int i = 0; i < SEC_TPM_SALT_LEN; i++) {
		SEC_ASSERT(sec_ctx.sec_ctx.a_hkdf_32b_tpm_salt[i] == 0xaa);
	}

	SEC_ASSERT(
		sec_ctx.sec_ctx.hkdf_32b_tpm_salt_len == SEC_TPM_SALT_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_get_hkdf_32b_pse_salt(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_PSE_SALT_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_PSE_SALT_LEN; i++) {
		sec_ctx.sec_ctx.a_hkdf_32b_pse_salt[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.hkdf_32b_pse_salt_len = SEC_PSE_SALT_LEN;
	sec_len = SEC_PSE_SALT_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_get_hkdf_32b_pse_salt(
			    a_sec_data, &sec_len) == SEC_SUCCESS));

	for (int i = 0; i < SEC_PSE_SALT_LEN; i++) {
		SEC_ASSERT(a_sec_data[i] == 0xff);
	}

	SEC_ASSERT(sec_len == SEC_PSE_SALT_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_set_hkdf_32b_pse_salt(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_PSE_SALT_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_PSE_SALT_LEN; i++) {
		sec_ctx.sec_ctx.a_hkdf_32b_pse_salt[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.hkdf_32b_pse_salt_len = SEC_PSE_SALT_LEN;
	sec_len = SEC_PSE_SALT_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_set_hkdf_32b_pse_salt(
			    a_sec_data, &sec_len) == SEC_SUCCESS));

	for (int i = 0; i < SEC_PSE_SALT_LEN; i++) {
		SEC_ASSERT(sec_ctx.sec_ctx.a_hkdf_32b_pse_salt[i] == 0xaa);
	}

	SEC_ASSERT(
		sec_ctx.sec_ctx.hkdf_32b_pse_salt_len == SEC_PSE_SALT_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_get_device_id(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_DEV_ID_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_DEV_ID_LEN; i++) {
		sec_ctx.sec_ctx.a_device_id[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.device_id_len = SEC_DEV_ID_LEN;
	sec_len = SEC_DEV_ID_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_get_device_id(
			    a_sec_data, &sec_len) == SEC_SUCCESS));

	for (int i = 0; i < SEC_DEV_ID_LEN; i++) {
		SEC_ASSERT(a_sec_data[i] == 0xff);
	}

	SEC_ASSERT(sec_len == SEC_DEV_ID_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_set_device_id(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_DEV_ID_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_DEV_ID_LEN; i++) {
		sec_ctx.sec_ctx.a_device_id[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.device_id_len = SEC_DEV_ID_LEN;
	sec_len = SEC_DEV_ID_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_set_device_id(
			    a_sec_data, &sec_len) == SEC_SUCCESS));

	for (int i = 0; i < SEC_DEV_ID_LEN; i++) {
		SEC_ASSERT(sec_ctx.sec_ctx.a_device_id[i] == 0xaa);
	}

	SEC_ASSERT(sec_ctx.sec_ctx.device_id_len == SEC_DEV_ID_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_get_token_id(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_TOK_ID_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_TOK_ID_LEN; i++) {
		sec_ctx.sec_ctx.a_token_id[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.token_id_len = SEC_TOK_ID_LEN;
	sec_len = SEC_TOK_ID_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_get_token_id(
			    a_sec_data, &sec_len) == SEC_SUCCESS));

	for (int i = 0; i < SEC_TOK_ID_LEN; i++) {
		SEC_ASSERT(a_sec_data[i] == 0xff);
	}

	SEC_ASSERT(sec_len == SEC_TOK_ID_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_set_token_id(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_TOK_ID_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_TOK_ID_LEN; i++) {
		sec_ctx.sec_ctx.a_token_id[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.token_id_len = SEC_TOK_ID_LEN;
	sec_len = SEC_TOK_ID_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_set_token_id(
			    a_sec_data, &sec_len) == SEC_SUCCESS));

	for (int i = 0; i < SEC_TOK_ID_LEN; i++) {
		SEC_ASSERT(sec_ctx.sec_ctx.a_token_id[i] == 0xaa);
	}

	SEC_ASSERT(sec_ctx.sec_ctx.token_id_len == SEC_TOK_ID_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_get_cloud_host_url(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_URL_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() ==
		    SEC_SUCCESS));

	for (int i = 0; i < SEC_URL_LEN; i++) {
		sec_ctx.sec_ctx.a_cloud_host_url[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.cloud_host_url_len = SEC_URL_LEN;
	sec_len = SEC_URL_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_get_cloud_host_url(
			    a_sec_data, &sec_len) == SEC_SUCCESS));

	for (int i = 0; i < SEC_URL_LEN; i++) {
		SEC_ASSERT(a_sec_data[i] == 0xff);
	}

	SEC_ASSERT(sec_len == SEC_URL_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_set_cloud_host_url(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_URL_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_URL_LEN; i++) {
		sec_ctx.sec_ctx.a_cloud_host_url[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.cloud_host_url_len = SEC_URL_LEN;
	sec_len = SEC_URL_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_set_cloud_host_url(
			    a_sec_data, &sec_len) == SEC_SUCCESS));

	for (int i = 0; i < SEC_URL_LEN; i++) {
		SEC_ASSERT(sec_ctx.sec_ctx.a_cloud_host_url[i] == 0xaa);
	}

	SEC_ASSERT(
		sec_ctx.sec_ctx.cloud_host_url_len == SEC_URL_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_get_cloud_host_port(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned int sec_data;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	sec_ctx.sec_ctx.cloud_host_port = 0xff;
	sec_data = 0xaa;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_get_cloud_host_port(
			    &sec_data) == SEC_SUCCESS));

	SEC_ASSERT(sec_data == 0xff);

	return result;
}

SEC_RTN int unit_test_sec_int_set_cloud_host_port(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned int sec_data;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	sec_ctx.sec_ctx.cloud_host_port = 0xff;
	sec_data = 0xaa;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_set_cloud_host_port(
			    &sec_data) == SEC_SUCCESS));

	SEC_ASSERT(sec_ctx.sec_ctx.cloud_host_port == 0xaa);

	return result;
}

SEC_RTN int unit_test_sec_int_get_proxy_host_url(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_URL_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_URL_LEN; i++) {
		sec_ctx.sec_ctx.a_proxy_host_url[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.proxy_host_url_len = SEC_URL_LEN;
	sec_len = SEC_URL_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_get_proxy_host_url(
			    a_sec_data,
			    &sec_len
			    ) == SEC_SUCCESS));

	for (int i = 0; i < SEC_URL_LEN; i++) {
		SEC_ASSERT(a_sec_data[i] == 0xff);
	}

	SEC_ASSERT(sec_len == SEC_URL_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_set_proxy_host_url(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned char a_sec_data[SEC_URL_LEN];
	unsigned int sec_len;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	for (int i = 0; i < SEC_URL_LEN; i++) {
		sec_ctx.sec_ctx.a_proxy_host_url[i] = 0xff;
		a_sec_data[i] = 0xaa;
	}
	sec_ctx.sec_ctx.proxy_host_url_len = SEC_URL_LEN;
	sec_len = SEC_URL_LEN;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_set_proxy_host_url(
			    a_sec_data, &sec_len) == SEC_SUCCESS));

	for (int i = 0; i < SEC_URL_LEN; i++) {
		SEC_ASSERT(sec_ctx.sec_ctx.a_proxy_host_url[i] == 0xaa);
	}

	SEC_ASSERT(
		sec_ctx.sec_ctx.proxy_host_url_len == SEC_URL_LEN);

	return result;
}

SEC_RTN int unit_test_sec_int_get_proxy_host_port(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned int sec_data;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	sec_ctx.sec_ctx.proxy_host_port = 0xff;
	sec_data = 0xaa;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_get_proxy_host_port(
			    &sec_data) == SEC_SUCCESS));

	SEC_ASSERT(sec_data == 0xff);

	return result;
}

SEC_RTN int unit_test_sec_int_set_proxy_host_port(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned int sec_data;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	sec_ctx.sec_ctx.proxy_host_port = 0xff;
	sec_data = 0xaa;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_set_proxy_host_port(
			    &sec_data) == SEC_SUCCESS));

	SEC_ASSERT(
		sec_ctx.sec_ctx.proxy_host_port == 0xaa);

	return result;
}

SEC_RTN int unit_test_sec_int_get_provisioning_state(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned int sec_data;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	sec_ctx.sec_ctx.provisioning_state = SEC_PEND;
	sec_data = SEC_PROV;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_get_provisioning_state(
			    &sec_data) == SEC_SUCCESS));

	SEC_ASSERT(sec_data == SEC_PEND);

	return result;
}

SEC_RTN int unit_test_sec_int_set_provisioning_state(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned int sec_data;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	sec_ctx.sec_ctx.provisioning_state = SEC_PEND;
	sec_data = SEC_PROV;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_set_provisioning_state(
			    &sec_data) == SEC_SUCCESS));

	SEC_ASSERT(sec_ctx.sec_ctx.provisioning_state == SEC_PROV);

	return result;
}

SEC_RTN int unit_test_sec_int_get_reprovision_pending(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned int sec_data;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	sec_ctx.sec_ctx.reprovision_pending = SEC_REPROV_PEND;
	sec_data = (unsigned int) SEC_REPROV_NONE;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_get_reprovision_pending(
			    &sec_data) == SEC_SUCCESS));

	SEC_ASSERT(sec_data == (unsigned int) SEC_REPROV_PENDING);

	return result;
}

SEC_RTN int unit_test_sec_int_set_reprovision_pending(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned int sec_data;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	sec_ctx.sec_ctx.reprovision_pending = SEC_REPROV_PEND;
	sec_data = (unsigned int) SEC_REPROV_NONE;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_set_reprovision_pending(
			    &sec_data) == SEC_SUCCESS));

	SEC_ASSERT(
		sec_ctx.sec_ctx.reprovision_pending == SEC_REPROV_NONE);

	return result;
}

SEC_RTN int unit_test_sec_int_set_reprovision(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	unsigned int sec_data;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	sec_ctx.sec_ctx.provisioning_state = SEC_PEND;
	sec_data = SEC_PROV;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_set_reprovision(
			    &sec_data, sizeof(sec_data), SEC_PROV_STATE) == SEC_SUCCESS));

	SEC_ASSERT(
		sec_ctx.sec_ctx.provisioning_state == SEC_PROV);

	return result;
}

SEC_RTN int unit_test_sec_int_get_context(void)
{
	int result;

	unsigned int clear_text_size_for_hash;

	sec_context sec_data;

	result = SEC_SUCCESS;

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_init_context() == SEC_SUCCESS));

	sec_ctx.sec_ctx.proxy_host_port = 998;
	sec_data.proxy_host_port = 111;

	SEC_ASSERT((sec_int_update_context_hash() == SEC_SUCCESS));

	SEC_ASSERT((sec_int_get_context(&sec_data) == SEC_SUCCESS));

	SEC_ASSERT(sec_data.proxy_host_port == 998);

	return result;
}

SEC_RTN int unit_test_sec_int_wait_timeout_sync(void)
{
	int result;

	unsigned long prev_time = 0;
	unsigned long cur_time = 0;

	result = SEC_SUCCESS;

	sec_int_get_time(&prev_time);

	sec_int_wait_timeout_sync(10);

	sec_int_get_time(&cur_time);

	SEC_ASSERT((cur_time - prev_time) >= 10);

	return result;
}

SEC_RTN int unit_test_sec_int_get_time(void)
{
	int result;

	unsigned long prev_time = 0;
	unsigned long cur_time = 0;

	result = SEC_SUCCESS;

	sec_int_get_time(&prev_time);

	while (1) {
		sec_int_get_time(&cur_time);
		if (prev_time != cur_time) {
			break;
		}
	}

	SEC_ASSERT(prev_time != cur_time);

	return result;
}

unsigned int sec_timer_stop_flag;

SEC_RTN void sec_default_expire_func(
	struct k_timer *timer
	)
{
	k_timer_stop(timer);

	sec_timer_stop_flag = 1;
}

SEC_RTN int unit_test_sec_int_wait_timeout_async(void)
{
	int result;

	unsigned long prev_time = 0;
	unsigned long cur_time = 0;

	result = SEC_SUCCESS;

	sec_int_get_time(&prev_time);

	sec_timer_stop_flag = 0;

	sec_int_wait_timeout_async(10, sec_default_expire_func);

	/** The while loop is only needed for the unit test specific
	 * expiry function. It is waiting for the test flag in the custom
	 * expiry function get set to 1.It is not needed if there is no
	 * notification from the expiry function is needed in the calling
	 * function from OOB Agent.
	 */
	while (!sec_timer_stop_flag) {
		LOG_INF(".");
	}
	;

	LOG_INF("\n");

	sec_int_get_time(&cur_time);

	SEC_ASSERT((cur_time - prev_time) >= 10);

	return result;
}

SEC_RTN int unit_test_sec_int_get_rand_num(void)
{
	int result;

	unsigned int rand_num_data;

	result = SEC_SUCCESS;

	SEC_ASSERT((sec_int_get_rand_num(
			    &rand_num_data, SEC_DRNG_EGETDATA24_SIZE_BITS) == SEC_SUCCESS));
	SEC_ASSERT(rand_num_data == 0x01234567);

	return result;
}

SEC_RTN int unit_test_sec_int_get_rand_num_by_mbed(void)
{
	int result;

	unsigned int rand_num_data;

	result = SEC_SUCCESS;

	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_get_rand_num_by_mbed(
			    &rand_num_data, SEC_DRNG_EGETDATA24_SIZE_BITS) == SEC_SUCCESS));
	SEC_ASSERT(rand_num_data == 0x00f354ad);
	SEC_ASSERT(mbedtls_hardware_poll_flag = 1);

	return result;
}

SEC_RTN int unit_test_sec_int_get_nonce(void)
{
	int result;

	unsigned int nonce;

	result = SEC_SUCCESS;

	SEC_ASSERT((sec_int_get_nonce(&nonce, 0x5678) == SEC_SUCCESS));
	SEC_ASSERT(nonce == 0x01234567);
	SEC_ASSERT(sec_ctx.tran_id == 0x5678);

	return result;
}

SEC_RTN int unit_test_sec_int_check_nonce(void)
{
	int result;

	unsigned int nonce;
	unsigned char key[SEC_OWN_PUB_KEY_LEN + 1] =
		"KE123456789012345678901234567890";
	unsigned int key_len = SEC_OWN_PUB_KEY_LEN;

	unsigned char a_encrypted_nonce_iv
	[SEC_NONCE_DECRYPTED_IV_SIZE + 1] = "IV1234567890";
	unsigned char a_encrypted_nonce_tag
	[SEC_NONCE_DECRYPTED_TAG_SIZE + 1];
	unsigned int tran_id = 0x5678;
	unsigned int clear_text_size_for_hash;

	/* mbed */
	int ret;
	mbedtls_gcm_context gcm;

	unsigned char clear_text[SEC_NONCE_DECRYPTED_BUFFER_SIZE + 1];
	/* = "a secret message!"; */
	unsigned char encrypted[SEC_NONCE_DECRYPTED_BUFFER_SIZE + 1];
	unsigned char decrypted[SEC_NONCE_DECRYPTED_BUFFER_SIZE + 1];

	result = SEC_SUCCESS;

	memset((void *)clear_text, 0, sizeof(clear_text));
	memset((void *)encrypted, 0, sizeof(encrypted));
	memset((void *)decrypted, 0, sizeof(decrypted));

	clear_text_size_for_hash = sizeof(sec_ctx.sec_ctx);
	unit_test_param[0] = (void *)&clear_text_size_for_hash;

	SEC_ASSERT((sec_int_get_nonce(&nonce, tran_id) == SEC_SUCCESS));

	*((unsigned int *)clear_text) = nonce;

	SEC_ASSERT(sec_int_set_owner_public_key(key, &key_len) == SEC_SUCCESS);
	SEC_ASSERT(strncmp((char *)sec_ctx.sec_ctx.a_owner_public_key,
			   (char *)key, SEC_OWN_PUB_KEY_LEN) == 0);

	mbedtls_gcm_init(&gcm);

	ret = mbedtls_gcm_setkey(&gcm,
				 MBEDTLS_CIPHER_ID_AES,
				 key,
				 key_len * 8
				 );

	if (ret != 0) {
		return SEC_FAILED;
	}

	ret = mbedtls_gcm_crypt_and_tag(&gcm,
					MBEDTLS_GCM_ENCRYPT,
					SEC_NONCE_DECRYPTED_DATA_SIZE,
					a_encrypted_nonce_iv,
					SEC_NONCE_DECRYPTED_IV_SIZE,
					(unsigned char *)"",
					0,
					clear_text,
					encrypted,
					SEC_NONCE_DECRYPTED_TAG_SIZE,
					a_encrypted_nonce_tag);

	if (ret != 0) {
		return SEC_FAILED;
	}

	SEC_ASSERT((sec_int_check_nonce(
			    encrypted,
			    SEC_NONCE_DECRYPTED_DATA_SIZE,
			    a_encrypted_nonce_iv,
			    SEC_NONCE_DECRYPTED_IV_SIZE,
			    a_encrypted_nonce_tag,
			    SEC_NONCE_DECRYPTED_TAG_SIZE,
			    tran_id
			    ) == SEC_SUCCESS));

	return result;
}

SEC_RTN int main(void)
{
	int result;

	LOG_INF("Unit Test start!\n");

	result = SEC_SUCCESS;

	result = unit_test_sec_int_verify_context_hash();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_verify_context_hash()!\n");

	result = unit_test_sec_int_update_context_hash();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_update_context_hash()!\n");

	result = unit_test_sec_int_get_context_param();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_context_param()!\n");

	result = unit_test_sec_int_set_context_param();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_set_context_param()!\n");

	result = unit_test_sec_int_get_context_param_simple();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_context_param_simple()!\n");

	result = unit_test_sec_int_set_context_param_simple();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_set_context_param_simple()!\n");

	result = unit_test_sec_int_get_root_ca();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_root_ca!\n");

	result = unit_test_sec_int_set_root_ca();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_set_root_ca!\n");

	result = unit_test_sec_int_get_owner_public_key();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_owner_public_key!\n");

	result = unit_test_sec_int_set_owner_public_key();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_set_owner_public_key!\n");

	result = unit_test_sec_int_get_hkdf_32b_tpm_salt();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_hkdf_32b_tpm_salt!\n");

	result = unit_test_sec_int_set_hkdf_32b_tpm_salt();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_set_hkdf_32b_tpm_salt!\n");

	result = unit_test_sec_int_get_hkdf_32b_pse_salt();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_hkdf_32b_pse_salt!\n");

	result = unit_test_sec_int_set_hkdf_32b_pse_salt();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_set_hkdf_32b_pse_salt!\n");

	result = unit_test_sec_int_get_device_id();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_device_id!\n");

	result = unit_test_sec_int_set_device_id();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_set_device_id!\n");

	result = unit_test_sec_int_get_token_id();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_token_id!\n");

	result = unit_test_sec_int_set_token_id();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_set_token_id!\n");

	result = unit_test_sec_int_get_cloud_host_url();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_cloud_host_url!\n");

	result = unit_test_sec_int_set_cloud_host_url();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_set_cloud_host_url!\n");

	result = unit_test_sec_int_get_cloud_host_port();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_cloud_host_port!\n");

	result = unit_test_sec_int_set_cloud_host_port();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_set_cloud_host_port!\n");

	result = unit_test_sec_int_get_proxy_host_url();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_proxy_host_url!\n");

	result = unit_test_sec_int_set_proxy_host_url();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_set_proxy_host_url!\n");

	result = unit_test_sec_int_get_proxy_host_port();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_proxy_host_port!\n");

	result = unit_test_sec_int_set_proxy_host_port();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_set_proxy_host_port!\n");

	result = unit_test_sec_int_get_provisioning_state();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_provisioning_state!\n");

	result = unit_test_sec_int_set_provisioning_state();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_set_provisioning_state!\n");

	result = unit_test_sec_int_get_reprovision_pending();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_reprovision_pending!\n");

	result = unit_test_sec_int_set_reprovision_pending();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_set_reprovision_pending!\n");

	result = unit_test_sec_int_get_context();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_context!\n");

	result = unit_test_sec_int_wait_timeout_sync();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_wait_timeout_sync!\n");

	result = unit_test_sec_int_get_time();
	SEC_ASSERT(result == SEC_SUCCESS);

	result = unit_test_sec_int_wait_timeout_async();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_wait_timeout_async!\n");

	LOG_INF("unit_test_sec_int_get_time!\n");

	result = unit_test_sec_int_get_rand_num();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_rand_num!\n");

	result = unit_test_sec_int_get_rand_num_by_mbed();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_rand_num_by_mbed!\n");

	/* Fix this at SAM E70 */
	result = unit_test_sec_int_get_nonce();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_get_nonce!\n");

	/* Following two test cases got sequencing dependency */
	result = unit_test_sec_int_init_context();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_init_context()!\n");

	result = unit_test_sec_int_check_nonce();
	SEC_ASSERT(result == SEC_SUCCESS);

	LOG_INF("unit_test_sec_int_check_nonce!\n");

exit:

#if (defined(_WIN32) && _WIN32)
	LOG_INF("  + Press Enter to exit this program.\n");
	fflush(stdout); getchar();
#endif

	LOG_INF("Unit Test Done!\n");
	return result;
}


#endif /* #if (defined(SEC_UNIT_TEST) && SEC_UNIT_TEST) */

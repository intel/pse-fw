/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "driver/sedi_driver_sideband.h"

#include "pse_oob_sec_base.h"
#include "pse_oob_sec_internal.h"
#include "pse_oob_sec_status_code.h"
#include "pse_oob_sec_enum.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <kernel.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <sys/printk.h>

#include "mbedtls/sha512.h"     /** SHA-512 only */
#include "mbedtls/md.h"         /** generic interface */
#include "mbedtls/gcm.h"        /** mbedtls_gcm_context */
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#include "kernel.h"

#include <common/pse_app_framework.h>
#include <common/utils.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(SEC_INTERNAL, CONFIG_OOB_LOGGING);

#define IS384 1  /** 1-> SHA-384 0-> SHA-512 */
#if defined(CONFIG_SOC_INTEL_PSE)
APP_GLOBAL_VAR(2) struct sec_context_wrapper
	sec_ctx;
#else
struct sec_context_wrapper sec_ctx;
#endif

#define RETRY 2
#define COMPARE_DRNG_SEED(prev, cur) (prev ^ cur)

typedef struct {
	unsigned int drng_prev;
	bool is_1st_drng_rd;
	bool is_not_1st_hw_poll;
	bool entropy_state;
} SEC_INT_ENTROPY_T;
static SEC_INT_ENTROPY_T sec_entropy_t;

/** PRIVATE Function */
SEC_RTN int sec_int_verify_context_hash(void)
{
	unsigned int i;
	unsigned int input_text_size;
	unsigned char calculated_hash[SEC_HASH_LEN];

#if (defined(SEC_UNIT_TEST) && SEC_UNIT_TEST)
	input_text_size = *((unsigned int *)unit_test_param[0]);
#else
	input_text_size = sizeof(sec_ctx.sec_ctx);
#endif

	mbedtls_sha512((unsigned char *)&(sec_ctx.sec_ctx),
		       input_text_size,
		       calculated_hash,
		       IS384);

	for (i = 0; i < SEC_HASH_LEN; i++) {
		if (calculated_hash[i] != sec_ctx.a_hash[i]) {
			return SEC_FAILED_OOB_HASH_MISMATCH;
		}
	}

	return SEC_SUCCESS;
};

SEC_RTN int sec_int_update_context_hash(void)
{
	unsigned int input_text_size;

#if (defined(SEC_UNIT_TEST) && SEC_UNIT_TEST)
	input_text_size = *((unsigned int *)unit_test_param[0]);
#else
	input_text_size = sizeof(sec_ctx.sec_ctx);
#endif

	mbedtls_sha512((unsigned char *)&(sec_ctx.sec_ctx),
		       input_text_size,
		       sec_ctx.a_hash,
		       IS384);

	return SEC_SUCCESS;
};

SEC_RTN int sec_int_init_context(void)
{
	unsigned int result = SEC_SUCCESS;

#if (defined(SEC_UNIT_TEST) && SEC_UNIT_TEST)
	memset(((void *)&sec_ctx.sec_ctx),
	       *((unsigned int *)(unit_test_param[1])),
	       *((unsigned int *)unit_test_param[0]));
#else
	memset((void *)&sec_ctx.sec_ctx,
	       0,
	       sizeof(sec_ctx.sec_ctx));

	sec_ctx.sec_ctx.prov_state =
		SEC_PEND;

	sec_ctx.sec_ctx.reprov_pend =
		SEC_REPROV_NONE;
#endif

	/** Initialize Hash() */
	result = sec_int_update_context_hash();

	if (result != SEC_SUCCESS) {
		LOG_INF("%s failed!", __func__);
		return result;
	}

	return result;
};

SEC_RTN void sec_int_sec_decommission(void)
{
	oob_set_pm_control(false);
	memset((void *)&sec_ctx, 0, sizeof(sec_ctx));
};

/*
 * API to get size of string using strlen
 * to populate value in cred size
 */
SEC_RTN unsigned int sec_int_get_context_param_by_len(
	SEC_INOUT void            *p_sec_data,
	SEC_INOUT unsigned int    *p_sec_len,
	SEC_IN void            *p_ctx_param_data
	)
{

	size_t p_ctx_param_len = strlen(p_ctx_param_data);

	memmove(p_sec_data, p_ctx_param_data, p_ctx_param_len);
	*p_sec_len = p_ctx_param_len;

	return SEC_SUCCESS;
};

SEC_RTN unsigned int sec_int_get_context_param(
	SEC_INOUT void            *p_sec_data,
	SEC_INOUT unsigned int    *p_sec_len,
	SEC_IN void            *p_ctx_param_data,
	SEC_IN unsigned int    *p_ctx_param_len
	)
{

	memmove(p_sec_data, p_ctx_param_data, *p_ctx_param_len);
	*p_sec_len = *p_ctx_param_len;

	return SEC_SUCCESS;
};

SEC_RTN unsigned int sec_int_set_context_param(
	SEC_IN void            *p_sec_data,
	SEC_IN unsigned int    *p_sec_len,
	SEC_INOUT void            *p_ctx_param_data,
	SEC_INOUT unsigned int    *p_ctx_param_len
	)
{

	memmove(p_ctx_param_data, p_sec_data, *p_sec_len);
	*p_ctx_param_len = *p_sec_len;

	return SEC_SUCCESS;
};


SEC_RTN unsigned int sec_int_get_context_param_simple(
	SEC_OUT unsigned int *p_sec_data,
	SEC_OUT unsigned int *p_ctx_param_data
	)
{

	*p_sec_data = *p_ctx_param_data;

	return SEC_SUCCESS;
};

SEC_RTN unsigned int sec_int_set_context_param_simple(
	SEC_OUT unsigned int *p_sec_data,
	SEC_OUT unsigned int *p_ctx_param_data
	)
{

	*p_ctx_param_data = *p_sec_data;

	return SEC_SUCCESS;
};

SEC_RTN unsigned int sec_int_get_root_ca(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	if (sec_ctx.sec_ctx.root_ca_len >
	    SEC_ROOT_CA_LEN) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s param failed!", __func__);
		return SEC_FAILED_INVALID_PARAM;
	}

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	result = sec_int_get_context_param_by_len(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_root_ca
		);

	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_mqtt_client_id(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();
	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_mqtt_client_id,
		&sec_ctx.sec_ctx.a_mqtt_client_id_len
		);

	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_get_mqtt_client_id(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	result = sec_int_get_context_param_by_len(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_mqtt_client_id
		);

	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_root_ca(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	if (*p_sec_len > SEC_ROOT_CA_LEN) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s param failed!", __func__);
		return SEC_FAILED_INVALID_PARAM;
	}

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_root_ca,
		&sec_ctx.sec_ctx.root_ca_len
		);

	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_get_own_pub_key(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	result = sec_int_get_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_own_pub_key,
		&sec_ctx.sec_ctx.own_pub_key_len
		);

	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_own_pub_key(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int     *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_own_pub_key,
		&sec_ctx.sec_ctx.own_pub_key_len
		);

	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_pse_32b_fuse(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	if (*p_sec_len != SEC_FUSE_LEN) {
		return SEC_FAILED;
	}

	sec_int_lock_ctx_mutex();
	sec_int_set_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.a_pse_32b_fuse,
		&sec_ctx.a_pse_32b_fuse_len
		);

	sec_int_unlock_ctx_mutex();
	return result;
}

SEC_RTN unsigned int sec_int_get_pse_32b_fuse(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{

	unsigned int result = SEC_SUCCESS;

	if (*p_sec_len != sec_ctx.a_pse_32b_fuse_len) {
		return SEC_FAILED;
	}

	sec_int_lock_ctx_mutex();
	result = sec_int_get_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.a_pse_32b_fuse,
		&sec_ctx.a_pse_32b_fuse_len
		);

	sec_int_unlock_ctx_mutex();
	return result;
};

SEC_RTN unsigned int sec_int_get_hkdf_32b_pse_salt(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	if (sec_ctx.sec_ctx.hkdf_32b_pse_salt_len != SEC_PSE_SALT_LEN) {
		return SEC_FAILED;
	}

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	result = sec_int_get_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_hkdf_32b_pse_salt,
		&sec_ctx.sec_ctx.hkdf_32b_pse_salt_len
		);

	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_hkdf_32b_pse_salt(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_hkdf_32b_pse_salt,
		&sec_ctx.sec_ctx.hkdf_32b_pse_salt_len
		);

	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_get_dev_id(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	result = sec_int_get_context_param_by_len(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_dev_id
		);

	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_dev_id(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_dev_id,
		&sec_ctx.sec_ctx.dev_id_len
		);

	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_get_tok_id(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	result = sec_int_get_context_param_by_len(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_tok_id
		);
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_tok_id(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_tok_id,
		&sec_ctx.sec_ctx.tok_id_len
		);

	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;

};

SEC_RTN unsigned int sec_int_get_cld_adapter(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	result = sec_int_get_context_param_by_len(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_cld_adapter
		);

	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_cld_adapter(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_cld_adapter,
		&sec_ctx.sec_ctx.cld_adapter_len
		);

	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_get_cld_host_url(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	result = sec_int_get_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_cld_host_url,
		&sec_ctx.sec_ctx.cld_host_url_len
		);

	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_cld_host_url(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_cld_host_url,
		&sec_ctx.sec_ctx.cld_host_url_len
		);

	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_get_cld_host_port(
	SEC_OUT unsigned int     *p_sec_data
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	result = sec_int_get_context_param_simple(
		p_sec_data,
		(unsigned int *)&sec_ctx.sec_ctx
		.cld_host_port
		);
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_cld_host_port(
	SEC_OUT unsigned int     *p_sec_data
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param_simple(
		p_sec_data,
		(unsigned int *)&sec_ctx.sec_ctx
		.cld_host_port
		);

	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_get_pxy_host_url(
	SEC_OUT void     *p_sec_data,
	SEC_OUT unsigned int     *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	result = sec_int_get_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_pxy_host_url,
		&sec_ctx.sec_ctx.pxy_host_url_len
		);

	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_pxy_host_url(
	SEC_OUT void     *p_sec_data,
	SEC_OUT unsigned int     *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_pxy_host_url,
		&sec_ctx.sec_ctx.pxy_host_url_len
		);

	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_get_pxy_host_port(
	SEC_OUT unsigned int     *p_sec_data
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	result = sec_int_get_context_param_simple(
		p_sec_data,
		(unsigned int *)&sec_ctx.sec_ctx
		.pxy_host_port);

	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_pxy_host_port(
	SEC_OUT unsigned int     *p_sec_data
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param_simple(
		p_sec_data,
		(unsigned int *)&sec_ctx.sec_ctx
		.pxy_host_port);

	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_get_cloud_hash(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int     *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	if (sec_ctx.sec_ctx.a_cloud_hash_len != SEC_CLOUD_HASH_LEN) {
		return SEC_FAILED;
	}

	sec_int_lock_ctx_mutex();
	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	result = sec_int_get_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_cloud_hash,
		&sec_ctx.sec_ctx.a_cloud_hash_len
		);

	sec_int_unlock_ctx_mutex();
	return result;
};

SEC_RTN unsigned int sec_int_set_cloud_hash(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	if (*p_sec_len != SEC_CLOUD_HASH_LEN) {
		return SEC_FAILED_INVALID_PARAM_LEN;
	}

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_cloud_hash,
		&sec_ctx.sec_ctx.a_cloud_hash_len
		);

	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_cloud_hash_dec(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	)
{
	unsigned int result = SEC_SUCCESS;

	if (*p_sec_len != SEC_CLOUD_HASH_LEN) {
		return SEC_FAILED_INVALID_PARAM_LEN;
	}

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param(
		p_sec_data,
		p_sec_len,
		sec_ctx.sec_ctx.a_cloud_hash_dec,
		&sec_ctx.sec_ctx.a_cloud_hash_len
		);

	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_hc_int_hash_check(void)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();
	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	/* Hash value compare */
	for (int i = 0; i < SEC_CLOUD_HASH_LEN; i++) {
		if (sec_ctx.sec_ctx.a_cloud_hash[i]
		    != sec_ctx.sec_ctx.a_cloud_hash_dec[i]) {
			sec_int_unlock_ctx_mutex();
			return SEC_FAILED_OOB_HASH_MISMATCH;
		}
	}

	LOG_INF("sec_ctx.sec_ctx.a_cloud_hash\n");
	for (int i = 0; i < SEC_CLOUD_HASH_LEN; i++) {
		LOG_INF("%02x ", sec_ctx.sec_ctx.a_cloud_hash[i]);
	}
	LOG_INF("\nsec_ctx.sec_ctx.a_cloud_hash_dec\n");
	for (int i = 0; i < SEC_CLOUD_HASH_LEN; i++) {
		LOG_INF("%02x ", sec_ctx.sec_ctx.a_cloud_hash_dec[i]);
	}

	sec_int_unlock_ctx_mutex();
	return result;
};

SEC_RTN unsigned int sec_int_get_prov_state(
	SEC_OUT unsigned int *p_sec_data
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	result = sec_int_get_context_param_simple(
		p_sec_data,
		(unsigned int *)&sec_ctx.sec_ctx
		.prov_state
		);

	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_prov_state(
	SEC_OUT unsigned int     *p_sec_data
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param_simple(
		p_sec_data,
		(unsigned int *)&sec_ctx.sec_ctx
		.prov_state
		);

	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_get_reprov_pend(
	SEC_OUT unsigned int     *p_sec_data
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	result = sec_int_get_context_param_simple(
		p_sec_data,
		(unsigned int *)&sec_ctx.sec_ctx
		.reprov_pend
		);

	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_set_reprov_pend(
	SEC_OUT unsigned int     *p_sec_data
	)
{
	unsigned int result = SEC_SUCCESS;

	sec_int_lock_ctx_mutex();

	result = sec_int_verify_context_hash();

	if (result != SEC_SUCCESS) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return result;
	}

	sec_int_set_context_param_simple(
		p_sec_data,
		(unsigned int *)&sec_ctx.sec_ctx
		.reprov_pend
		);
	sec_int_update_context_hash();
	sec_int_unlock_ctx_mutex();

	return result;
};

/** This additional function is created per request of the OOB Agent
 * aimed for fast buffer copying of the full AEONRF content.
 * The validation of the content is expected to be done at the
 * calling function. It is recommended to use the individual
 * AEONRF parameter get() function for validated access.
 * Another note is the AEONRF internal physical structure mapping may
 * changed per request and corresponding update may be needed in
 * the calling function from OOB Agent. It is advice to use
 * the individual param get() function for a proper abstraction and forward
 * compatibility.
 */
SEC_RTN unsigned int sec_int_get_context(
	SEC_OUT struct sec_context *p_sec_data
	)
{

	*p_sec_data = sec_ctx.sec_ctx;

	return SEC_SUCCESS;
};

/** ========================= IOSF SIDEBAND STUB =========================*/

SEC_RTN unsigned int sec_int_open_iosf_sb(
	SEC_INOUT struct sec_iosf_sb_context *p_ctx
	)
{

	/* PSE IOSF SB Driver do not need open() phase, keep for future
	 * extension
	 */

	return SEC_SUCCESS;
};

SEC_RTN unsigned int sec_int_check_iosf_sb_ready(
	SEC_IN struct sec_iosf_sb_context *p_ctx
	)
{

	/* PSE IOSF SB Driver do not need ready() phase, keep for future
	 * extension
	 */

	return SEC_SUCCESS;
};

SEC_RTN unsigned int sec_int_read_iosf_sb(
	SEC_IN struct sec_iosf_sb_context *p_ctx,
	SEC_IN unsigned int port,
	SEC_IN unsigned int addr,
	SEC_INOUT void *p_sec_data,
	SEC_INOUT unsigned int *p_sec_len,
	SEC_IN unsigned int is_blocking,
	SEC_IN unsigned int blocking_timeout,
	SEC_IN unsigned int (*p_non_blocking_callback)(void *)
	)
{
	unsigned int read_val;
	int ret;

	switch (port) {
	case SB_DRNG:

#if (defined(SEC_UNIT_TEST) && SEC_UNIT_TEST)
		*((unsigned int *)p_sec_data) = 0x01234567;
#else

		ret = sedi_sideband_send(0, port, SEDI_SIDEBAND_ACTION_READ,
					 addr, 0);

		if (ret != 0) {
			LOG_INF("%s failed\n", __func__);
			return SEC_FAILED;
		}

		ret = sedi_sideband_wait_ack(0, port, SEDI_SIDEBAND_ACTION_READ,
					     &read_val);

		if (ret != 0) {
			LOG_INF("%s failed\n", __func__);
			return SEC_FAILED;
		}

		*((unsigned int *)p_sec_data) = read_val;

#endif

		*p_sec_len = 1;
		break;

	default:
		LOG_INF("%s: SEC_FAILED_INVALID_PARAM failed\n", __func__);
		return SEC_FAILED_INVALID_PARAM;
	}

	return SEC_SUCCESS;
};

SEC_RTN unsigned int sec_int_close_iosf_sb(
	SEC_IN struct sec_iosf_sb_context *p_ctx
	)
{

	/* PSE IOSF SB Driver do not need close() phase, keep for future
	 * extension
	 */

	return SEC_SUCCESS;
};

/** ========================= IOSF SIDEBAND STUB END ======================== */


/** ========================= Reprovision =================================== */

SEC_RTN unsigned int sec_int_set_reprovision(
	SEC_IN void *p_sec_data,
	SEC_IN unsigned int sec_len,
	SEC_IN enum sec_info_type type
	)
{

	enum reprov_pend_flag PENDING_SET = SEC_REPROV_PEND;

	switch (type) {
	case SEC_DEV_ID:
		sec_int_set_dev_id(p_sec_data, &sec_len);
		sec_int_set_reprov_pend((unsigned int *)&PENDING_SET);
		break;

	case SEC_TOK_ID:
		sec_int_set_tok_id(p_sec_data, &sec_len);
		sec_int_set_reprov_pend((unsigned int *)&PENDING_SET);
		break;

	case SEC_CLD_HOST_URL:
		sec_int_set_cld_host_url(p_sec_data, &sec_len);
		sec_int_set_reprov_pend((unsigned int *)&PENDING_SET);
		break;

	case SEC_CLD_HOST_PORT:
		sec_int_set_cld_host_port((unsigned int *)p_sec_data);
		sec_int_set_reprov_pend((unsigned int *)&PENDING_SET);
		break;

	case SEC_PXY_HOST_URL:
		sec_int_set_pxy_host_url(p_sec_data, &sec_len);
		sec_int_set_reprov_pend((unsigned int *)&PENDING_SET);
		break;

	case SEC_PXY_HOST_PORT:
		sec_int_set_pxy_host_port((unsigned int *)p_sec_data);
		sec_int_set_reprov_pend((unsigned int *)&PENDING_SET);
		break;

	case SEC_PROV_STATE:
		sec_int_set_prov_state((unsigned int *)p_sec_data);
		sec_int_set_reprov_pend((unsigned int *)&PENDING_SET);
		break;

	default:
		return SEC_FAILED_INVALID_PARAM;
	}

	return SEC_SUCCESS;
};




/** ========================= Handshake with BIOS ==========================*/

/* Function:    sec_hc_reset_drng_state, reset to default value
 *
 * return:      void
 */
static void sec_hc_reset_drng_state(void)
{
	sec_entropy_t.is_1st_drng_rd = false;
	sec_entropy_t.is_not_1st_hw_poll = false;
	sec_entropy_t.entropy_state = false;
	sec_entropy_t.drng_prev &= 0x0000;
};

unsigned int sec_int_start_init_flag;

SEC_RTN unsigned int sec_int_start(
	SEC_IN enum sec_mode mode,
	SEC_IN unsigned int timeout
	)
{
	unsigned int result = SEC_SUCCESS;

	unsigned long prev_time = 0;
	unsigned long cur_time = 0;
	unsigned long counter = 0;

	result = SEC_SUCCESS;

	/* reset drng logic states to default */
	sec_hc_reset_drng_state();

	sec_int_get_time(&prev_time);

	LOG_INF("%s: start\n", __func__);

	cur_time = prev_time;

	while ((cur_time - prev_time) < timeout) {

		if (counter < 5) {
			LOG_INF(".");
			counter++;
		} else {
			LOG_INF("\n");
			counter = 0;
			k_yield();
			k_sleep(K_MSEC(APP_MINI_LOOP_TIME));
		}

		if (sec_int_start_init_flag) {
			LOG_INF("sec_int_start_init_flag == 1: %ld\n",
				cur_time - prev_time);
			LOG_INF("%s: BIOS IPC received\n", __func__);
			break;
		}

		sec_int_get_time(&cur_time);
	}

	if ((cur_time - prev_time) > timeout) {
		LOG_INF("se_oob_sec_int_start() timeout: %ld\n",
			cur_time - prev_time);
		result = SEC_FAILED;
	}

	return result;
};

/** ========================= Random Number Generation ===================== */

SEC_RTN unsigned int sec_int_get_rand_num(
	SEC_INOUT unsigned int *p_rand_num_data,
	SEC_IN unsigned int size_in_bit
	)
{
	unsigned int result = SEC_SUCCESS;
	unsigned int len;
	struct sec_iosf_sb_context iosf_sb_ctx;
	unsigned int i;

	if (size_in_bit != SEC_DRNG_EGETDATA24_SIZE_BITS) {
		LOG_INF(
			"%s failed with size_in_bit != DRNG_EGETDATA24_SIZE_BITS\n",
			__func__);
		return SEC_FAILED;
	}

	result = sec_int_open_iosf_sb((void *)&iosf_sb_ctx);
	if (result != SEC_SUCCESS) {
		LOG_INF("%s failed\n", __func__);
		return result;
	}

	i = 0;
	while ((sec_int_check_iosf_sb_ready((void *)&iosf_sb_ctx) !=
		SEC_SUCCESS) &&
	       (i < SEC_DRNG_EGETDATA24_TIMEOUT_CYCLES)) {
		i++;
	}

	len = sizeof((*p_rand_num_data));
	result = sec_int_read_iosf_sb(
		(void *)&iosf_sb_ctx,
		SB_DRNG,
		SEC_DRNG_EGETDATA24_REG_ADDR,
		p_rand_num_data,
		&len,
		SEC_TRUE,
		SEC_DRNG_EGETDATA24_TIMEOUT_MS,
		NULL);
	if (result != SEC_SUCCESS) {
		LOG_INF("%s failed!", __func__);
		return result;
	}

	result = sec_int_close_iosf_sb((void *)&iosf_sb_ctx);

	return result;
};

#if (defined(SEC_UNIT_TEST) && SEC_UNIT_TEST)

/** Internal testing only to ensure that whenever MBEDTLS
 * is called for random number generation, the EHL hardware DRNG is used
 */

SEC_RTN unsigned int sec_int_get_rand_num_by_mbed(
	SEC_INOUT unsigned int *p_rand_num_data,
	SEC_IN unsigned int size_in_bit
	)
{
	unsigned int result = SEC_SUCCESS;

	mbedtls_entropy_context entropy;

	mbedtls_entropy_init(&entropy);

	mbedtls_ctr_drbg_context ctr_drbg;
	char *personalization = "SEC_INTERNAL";

	mbedtls_ctr_drbg_init(&ctr_drbg);

	result = mbedtls_ctr_drbg_seed(&ctr_drbg,
				       mbedtls_entropy_func,
				       &entropy,
				       (const unsigned char *)personalization,
				       strlen(personalization));

	if (result != SEC_SUCCESS) {
		LOG_INF("%s mbedtls_ctr_drbg_seed failed!", __func__);
		return result;
	}

	result = mbedtls_ctr_drbg_random(&ctr_drbg,
					 (unsigned char *)p_rand_num_data, 3);

	if (result != SEC_SUCCESS) {
		LOG_INF("%s mbedtls_ctr_drbg_random() failed!", __func__);
		return result;
	}

	return result;
};

#endif

/** ========================= NONCE Challenge ============================= */

SEC_RTN unsigned int sec_int_get_nonce(
	SEC_INOUT unsigned int *p_nonce_data,
	SEC_IN unsigned int tran_id
	)
{

	unsigned int result = SEC_SUCCESS;
	unsigned long time_in_s;

	sec_int_lock_ctx_mutex();
	if (tran_id < sec_ctx.tran_id) {
		LOG_INF("%s hash failed!", __func__);
		return SEC_FAILED_INVALID_PARAM;
	}
	sec_ctx.tran_id = tran_id;

	result = sec_int_get_rand_num(p_nonce_data,
				      SEC_DRNG_EGETDATA24_SIZE_BITS);

	sec_ctx.nonce = *p_nonce_data;

	sec_int_get_time(&time_in_s);
	sec_ctx.nonce_time_s = time_in_s;

	sec_int_unlock_ctx_mutex();

	return result;
};

SEC_RTN unsigned int sec_int_check_nonce(
	SEC_IN unsigned char *p_encrypted_nonce_data,
	SEC_IN unsigned int encrypted_nonce_data_len,
	SEC_IN unsigned char *p_encrypted_nonce_iv,
	SEC_IN int encrypted_nonce_iv_len,
	SEC_IN unsigned char *p_encrypted_nonce_tag,
	SEC_IN unsigned int encrypted_nonce_tag_len,
	SEC_IN unsigned int tran_id
	)
{

	unsigned int result = SEC_SUCCESS;
	unsigned char key[SEC_OWN_PUB_KEY_LEN];
	unsigned int key_len;
	unsigned long time_in_s;

	/* mbed */
	int ret;
	mbedtls_gcm_context gcm;
	unsigned char decrypted[SEC_NONCE_DENC_BUFFER_SIZE];

	sec_int_lock_ctx_mutex();
	if (tran_id != sec_ctx.tran_id) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s hash failed!", __func__);
		return SEC_FAILED_INVALID_PARAM;
	}
	sec_int_unlock_ctx_mutex();

	key_len = SEC_OWN_PUB_KEY_LEN;
	result = sec_int_get_own_pub_key(key, &key_len);

	if (result != SEC_SUCCESS) {
		LOG_INF("%s sec_int_get_own_pub_key() failed!", __func__);
		return result;
	}

	mbedtls_gcm_init(&gcm);

	ret = mbedtls_gcm_setkey(&gcm,
				 MBEDTLS_CIPHER_ID_AES,
				 key,
				 key_len * 8
				 );

	if (ret != 0) {
		LOG_INF("%s mbedtls_gcm_setkey() failed!", __func__);
		return SEC_FAILED;
	}

	ret = mbedtls_gcm_auth_decrypt(
		&gcm,
		encrypted_nonce_data_len,
		p_encrypted_nonce_iv,
		encrypted_nonce_iv_len,
		(unsigned char *)"",
		0,
		p_encrypted_nonce_tag,
		encrypted_nonce_tag_len,
		p_encrypted_nonce_data,
		decrypted);

	if (ret != 0) {
		LOG_INF("%s sec_int_get_own_pub_key() failed!", __func__);
		return SEC_FAILED;
	}

	sec_int_lock_ctx_mutex();
	if (sec_ctx.nonce != *((unsigned int *)decrypted)) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s(nonce !=*((unsigned int *)decrypted)) failed!",
			__func__);
		return SEC_FAILED;
	}

	sec_int_get_time(&time_in_s);

	if ((time_in_s - sec_ctx.nonce_time_s) >
	    SEC_NONCE_DENC_TIMEOUT_IN_S) {
		sec_int_unlock_ctx_mutex();
		LOG_INF("%s(time_in_s - sec_ctx.nonce_time_s) failed!",
			__func__);
		return SEC_FAILED_OOB_TIMEOUT;
	}
	sec_int_unlock_ctx_mutex();

	return result;
};

/** ========================= Mutex and Timeout Delay  =================== */

/**
 * See
 * http://docs.zephyrproject.org/1.6.0/kernel_v2/synchronization/mutexes.html
 */
K_MUTEX_DEFINE(sec_ctx_mutex);

SEC_RTN unsigned int sec_int_lock_ctx_mutex(void)
{

#if (defined(SEC_MULTI_THREAD_TASK_EN) && \
	SEC_MULTI_THREAD_TASK_EN)
	k_mutex_lock(&sec_ctx_mutex, K_FOREVER);
#endif

	return SEC_SUCCESS;
}

SEC_RTN unsigned int sec_int_unlock_ctx_mutex(void)
{

#if (defined(SEC_MULTI_THREAD_TASK_EN) && \
	SEC_MULTI_THREAD_TASK_EN)
	k_mutex_unlock(&sec_ctx_mutex);
#endif

	return SEC_SUCCESS;
};

/**
 *
 *
 * See See http://docs.zephyrproject.org/kernel/timing/timers.html
 */

struct k_timer sec_async_timer;

SEC_RTN unsigned int sec_int_wait_timeout_async(
	SEC_IN unsigned int ms,
	SEC_IN EXPIRY_FUNC expiry_func
	)
{

	k_timer_init(&sec_async_timer, expiry_func, NULL);
	k_timer_start(&sec_async_timer, K_MSEC(ms), K_NO_WAIT);

	return SEC_SUCCESS;
};

K_TIMER_DEFINE(sec_sync_timer, NULL, NULL);

SEC_RTN unsigned int sec_int_wait_timeout_sync(
	SEC_IN unsigned int ms
	)
{

	k_timer_start(&sec_sync_timer, K_MSEC(ms), K_NO_WAIT);

	k_timer_status_sync(&sec_sync_timer);

	return SEC_SUCCESS;
};

SEC_RTN unsigned int sec_int_get_time(
	SEC_IN unsigned long *time_s
	)
{
	/* (unsigned long)time(NULL); */

	*time_s = (unsigned long) k_uptime_get();

	return SEC_SUCCESS;
};

bool sec_hc_get_entropy_state(void)
{
	return sec_entropy_t.entropy_state;
};

/*
 * Implement the mbedtls_hardware_poll function to let it access the
 * device’s entropy source.
 *
 * See https://docs.mbed.com/docs/mbed-os-handbook/en/5.2/advanced/tls_porting/
 * See https://tls.mbed.org/kb/how-to/add-entropy-sources-to-entropy-pool
 * See https://tls.mbed.org/kb/how-to/add-a-random-generator
 * See https://tls.mbed.org/entropy-source-code
 */
int mbedtls_hardware_poll(
	void *data,
	unsigned char *output,
	size_t len,
	size_t *olen
	)
{
	LOG_INF("\nEnter: %s\n", __func__);

	unsigned int output_byte_cnt;
	int attempt = RETRY;
	unsigned int hw_entropy_buf = 0;
	unsigned int hw_entropy_byte_pos = 0;

  #if (defined(SEC_UNIT_TEST) && SEC_UNIT_TEST)
	*((unsigned int *)unit_test_param[0]) = 1;
  #endif

	/*
	 * Sample random number generation steps with security check with
	 *
	 * Caller request random number of 2 bytes 1st time after boot:
	 * A > B > D > F
	 *
	 * Caller request random number of 6 bytes 1st time after boot:
	 * A > B > D > E > F
	 *
	 * Caller request random number of another 6 bytes not first
	 * time after boot:
	 * A > B > C > D > E > F
	 *
	 * Caller request random number of another 3 bytes not first
	 * time after boot:
	 * A > B > C > D > F
	 *
	 */

	do {
		LOG_INF("Entropy: Default Attempts is : %d\n",
			attempt);

		hw_entropy_buf = 0;
		hw_entropy_byte_pos = SEC_BYTE3;

		/*
		 * Step A: Byte reqest loop for given length
		 * new drng read will fill 3bytes out of
		 * requested length
		 */
		for (
			output_byte_cnt = 0;
			output_byte_cnt < len;
			output_byte_cnt++
			) {
			if (hw_entropy_byte_pos > SEC_BYTE2) {
				hw_entropy_byte_pos = 0;
				/*
				 * Step B: Generates drng random number
				 */
				if (sec_int_get_rand_num(
					    &hw_entropy_buf,
					    SEC_DRNG_EGETDATA24_SIZE_BITS) ==
				    SEC_FAILED) {
					LOG_INF("%s failed\n", __func__);
					return SEC_FAILED;
				}
				hw_entropy_buf = hw_entropy_buf &
						 SEC_BYTE012_SET_MSK;
				/*
				 * Step C: 2nd request from caller to generate first
				 * Random after first hardware poll completed
				 * after boot
				 */
				if ((sec_entropy_t.is_1st_drng_rd == false) &&
				    (sec_entropy_t.is_not_1st_hw_poll ==
				     true)) {
					if (!COMPARE_DRNG_SEED(
						    sec_entropy_t.drng_prev,
						    hw_entropy_buf)) {
						/*
						 * HW Poll default attempt is set to 2,
						 * reaching here means first attempt is over,
						 * updating remaining last retry drng read
						 * request
						 */
						attempt = attempt - 1;
						sec_entropy_t.is_1st_drng_rd = false;
						if (attempt <= 0) {
							sec_entropy_t.entropy_state = false;
							LOG_DBG("Failed seeding, init status:%d\n"
								,
								sec_entropy_t.entropy_state);
							return SEC_FAILED;
						}
						goto retry;
					}
				}
				/*
				 * Step D: store the first hardware drng read from
				 * current call by caller for next compare with next
				 * hw drng read
				 */
				if (sec_entropy_t.is_1st_drng_rd == false) {
					sec_entropy_t.drng_prev =
						hw_entropy_buf;
				}
				/*
				 * Step E: Compare 2nd hw drng Read with 1st hw drng
				 * read store if prev and cur drng random num are
				 * different
				 */
				if (sec_entropy_t.is_1st_drng_rd == true) {
					if (COMPARE_DRNG_SEED(
						    sec_entropy_t.drng_prev,
						    hw_entropy_buf)) {
						sec_entropy_t.drng_prev =
							hw_entropy_buf;
					} else {
						/*
						 * HW Poll default attempt is set to 2,
						 * reaching here means first attempt is
						 * over , updating remaining last retry
						 * drng read request
						 */
						attempt = attempt - 1;
						sec_entropy_t.is_1st_drng_rd = false;
						if (attempt <= 0) {
							sec_entropy_t.entropy_state = false;
							LOG_DBG("Failed seeding, status: %d\n"
								,
								sec_entropy_t.entropy_state);
							return SEC_FAILED;
						}
						goto retry;
					}
				}
				/* set to true, once first
				 * drng read completed
				 */
				sec_entropy_t.is_1st_drng_rd = true;
			}

			/*
			 * Step: F output buf is copied with 24-bits/3bytes
			 * New Random num request is called after output is
			 * copied with 24bits/3bytes
			 */
			output[output_byte_cnt] =
				(unsigned char)(hw_entropy_buf);
			hw_entropy_buf = (hw_entropy_buf >> 8);
			hw_entropy_byte_pos++;
		} /* for loop */

retry:
		if (attempt == 2) {
			/*
			 * bypass pass case
			 */
			attempt = 0;
		}
	} while (attempt > 0); /* retry loop */

	*olen = len;

	/*
	 * Set entropy state is success, make 1st drng
	 * read as default, as first hw poll is successful
	 * set its as true
	 */
	sec_entropy_t.entropy_state = true;
	sec_entropy_t.is_1st_drng_rd = false;
	sec_entropy_t.is_not_1st_hw_poll = true;
	LOG_INF("Entropy: Succeeded: %d\n", sec_entropy_t.entropy_state);
	return SEC_SUCCESS;
};

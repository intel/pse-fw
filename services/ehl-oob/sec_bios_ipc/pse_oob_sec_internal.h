/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifndef PSE_OOB_SEC_INTERNAL_H
#define PSE_OOB_SEC_INTERNAL_H

#include "pse_oob_sec_base.h"
#include "pse_oob_sec_enum.h"

#include <zephyr/types.h>
#include <stddef.h>
#include <kernel.h>

#include <common/pse_app_framework.h>


typedef void (*EXPIRY_FUNC)(struct k_timer *a);

/*
 * Include Standard C Library
 * ==========================================================================
 */

/*
 * PSE OOB SEC AON RF
 */

#define SEC_VERSION_MAJOR
#define SEC_VERSION_MINOR

#define SEC_ROOT_CA_LEN 2048
#define SEC_OWN_PUB_KEY_LEN 384
#define SEC_PSE_SALT_LEN 32
#define SEC_DEV_ID_LEN 128
#define SEC_TOK_ID_LEN 192
#define SEC_MQTT_CLIENT_ID_LEN 64
#define SEC_CLOUD_HASH_LEN 48
#define SEC_CLD_LEN 32
#define SEC_URL_LEN 1024
#define SEC_PORT_LEN 2
#define SEC_HASH_LEN 48
#define SEC_FUSE_LEN 32
#define SEC_CSE_SEED_LEN 32

#define SEC_DRNG_EGETDATA24_REG_ADDR 0x0054
#define SEC_DRNG_EGETDATA24_TIMEOUT_MS 10
#define SEC_DRNG_EGETDATA24_TIMEOUT_CYCLES 65536
#define SEC_DRNG_EGETDATA24_SIZE_BITS 24

#define SEC_ENC_IV_SIZE 12
#define SEC_ENC_TAG_SIZE 16

#define SEC_NONCE_DENC_DATA_SIZE 16
#define SEC_NONCE_DENC_BUFFER_SIZE 128
#define SEC_NONCE_DENC_IV_SIZE 12
#define SEC_NONCE_DENC_TAG_SIZE 16
#define SEC_NONCE_DENC_TIMEOUT_IN_S 30

struct sec_context {
	unsigned char a_root_ca[SEC_ROOT_CA_LEN];
	unsigned int root_ca_len;
	unsigned char a_own_pub_key[SEC_OWN_PUB_KEY_LEN];
	unsigned int own_pub_key_len;
	unsigned char a_hkdf_32b_pse_salt[SEC_PSE_SALT_LEN];
	unsigned int hkdf_32b_pse_salt_len;
	unsigned char a_dev_id[SEC_DEV_ID_LEN];
	unsigned int dev_id_len;
	unsigned char a_tok_id[SEC_TOK_ID_LEN];
	unsigned int tok_id_len;
	unsigned char a_cld_adapter[SEC_CLD_LEN];
	unsigned int cld_adapter_len;
	unsigned char a_cld_host_url[SEC_URL_LEN];
	unsigned int cld_host_url_len;
	unsigned int cld_host_port;
	unsigned char a_pxy_host_url[SEC_URL_LEN];
	unsigned int pxy_host_url_len;
	unsigned char a_mqtt_client_id[SEC_MQTT_CLIENT_ID_LEN];
	unsigned int a_mqtt_client_id_len;
	unsigned char a_cloud_hash[SEC_CLOUD_HASH_LEN];
	unsigned char a_cloud_hash_dec[SEC_CLOUD_HASH_LEN];
	unsigned int a_cloud_hash_len;
	unsigned int pxy_host_port;
	unsigned int prov_state;
	/* boot flag */
	unsigned int reprov_pend;
};

struct sec_context_wrapper {
	struct sec_context sec_ctx;
	/* integrity check */
	unsigned char a_hash[SEC_HASH_LEN];
	unsigned int tran_id;
	unsigned int nonce;
	unsigned long nonce_time_s;

	/* encrypted data*/
	unsigned char a_enc_tok_id[SEC_TOK_ID_LEN];
	unsigned char a_enc_tok_id_iv[SEC_ENC_IV_SIZE];
	unsigned char a_enc_tok_id_tag[SEC_ENC_TAG_SIZE];

	unsigned char a_enc_dev_id[SEC_DEV_ID_LEN];
	unsigned char a_enc_dev_id_iv[SEC_ENC_IV_SIZE];
	unsigned char a_enc_dev_id_tag[SEC_ENC_TAG_SIZE];

	unsigned char a_enc_mqtt_client_id[SEC_MQTT_CLIENT_ID_LEN];
	unsigned char a_enc_mqtt_client_id_iv[SEC_ENC_IV_SIZE];
	unsigned char a_enc_mqtt_client_id_tag[SEC_ENC_TAG_SIZE];

	unsigned char a_enc_cloud_hash_id[SEC_CLOUD_HASH_LEN];
	unsigned char a_enc_cloud_hash_id_iv[SEC_ENC_IV_SIZE];
	unsigned char a_enc_cloud_hash_id_tag[SEC_ENC_TAG_SIZE];

	/* keys */
	unsigned char a_dev_id_enc_key[SEC_FUSE_LEN];
	unsigned char a_tok_id_enc_key[SEC_FUSE_LEN];
	unsigned char a_mqtt_client_id_enc_key[SEC_FUSE_LEN];
	unsigned char a_cloud_hash_enc_key[SEC_FUSE_LEN];

	/* fuse data*/
	unsigned char a_pse_32b_fuse[SEC_FUSE_LEN];
	unsigned int a_pse_32b_fuse_len;
};

typedef enum {
	CSE_SEED1       = 0,
	CSE_SEED2       = 1,
	CSE_SEED3       = 2,
	CSE_SEED4       = 3
} a_cse_seed_type;

struct sec_heci_context {
	unsigned int rsvd;
};

struct sec_iosf_sb_context {
	unsigned int rsvd;
};

SEC_RTN int sec_int_verify_context_hash(void);

SEC_RTN int sec_int_update_context_hash(void);

SEC_RTN int sec_int_init_context(void);

/* ================================================================= */

SEC_RTN unsigned int sec_int_get_context_param_by_len(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len,
	SEC_OUT void *p_ctx_param_data
	);

SEC_RTN unsigned int sec_int_get_context_param(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len,
	SEC_OUT void *p_ctx_param_data,
	SEC_OUT unsigned int *p_ctx_param_len
	);

SEC_RTN unsigned int sec_int_set_context_param(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len,
	SEC_OUT void *p_ctx_param_data,
	SEC_OUT unsigned int *p_ctx_param_len
	);


SEC_RTN unsigned int sec_int_get_context_param_simple(
	SEC_OUT unsigned int *p_sec_data,
	SEC_OUT unsigned int *p_ctx_param_data
	);

SEC_RTN unsigned int sec_int_set_context_param_simple(
	SEC_OUT unsigned int *p_sec_data,
	SEC_OUT unsigned int *p_ctx_param_data
	);


/* ================================================================= */

SEC_RTN unsigned int sec_int_get_root_ca(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

SEC_RTN unsigned int sec_int_set_root_ca(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

/* ================================================================= */

SEC_RTN unsigned int sec_int_get_own_pub_key(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

SEC_RTN unsigned int sec_int_set_own_pub_key(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

/* ================================================================= */

SEC_RTN unsigned int sec_int_get_hkdf_32b_pse_salt(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

SEC_RTN unsigned int sec_int_set_hkdf_32b_pse_salt(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

/* ================================================================= */

SEC_RTN unsigned int sec_int_set_pse_32b_fuse(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

SEC_RTN unsigned int sec_int_get_pse_32b_fuse(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

/* ================================================================= */

SEC_RTN unsigned int sec_int_get_dev_id(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

SEC_RTN unsigned int sec_int_set_dev_id(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

/* ================================================================= */

SEC_RTN unsigned int sec_int_get_tok_id(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

SEC_RTN unsigned int sec_int_set_tok_id(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

/* ================================================================ */

SEC_RTN unsigned int sec_int_set_mqtt_client_id(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

SEC_RTN unsigned int sec_int_get_mqtt_client_id(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

/* ================================================================= */

SEC_RTN unsigned int sec_int_get_cld_adapter(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

SEC_RTN unsigned int sec_int_set_cld_adapter(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

/* ================================================================= */

SEC_RTN unsigned int sec_int_get_cld_host_url(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

SEC_RTN unsigned int sec_int_set_cld_host_url(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

/* ================================================================= */

SEC_RTN unsigned int sec_int_get_cld_host_port(
	SEC_OUT unsigned int *p_sec_data
	);

SEC_RTN unsigned int sec_int_set_cld_host_port(
	SEC_OUT unsigned int *p_sec_data
	);

/* ================================================================= */

SEC_RTN unsigned int sec_int_get_pxy_host_url(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

SEC_RTN unsigned int sec_int_set_pxy_host_url(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

/* ================================================================= */

SEC_RTN unsigned int sec_int_get_pxy_host_port(
	SEC_OUT unsigned int *p_sec_data
	);

SEC_RTN unsigned int sec_int_set_pxy_host_port(
	SEC_OUT unsigned int *p_sec_data
	);

/* ================================================================= */

SEC_RTN unsigned int sec_int_get_reprov_pend(
	SEC_OUT unsigned int *p_sec_data
	);

SEC_RTN unsigned int sec_int_set_reprov_pend(
	SEC_OUT unsigned int *p_sec_data
	);

SEC_RTN unsigned int sec_int_get_prov_state(
	SEC_OUT unsigned int *p_sec_data
	);

SEC_RTN unsigned int sec_int_set_prov_state(
	SEC_OUT unsigned int *p_sec_data
	);

SEC_RTN unsigned int sec_int_get_context(
	SEC_OUT struct sec_context *p_sec_data
	);

/* =================================================================== */

SEC_RTN unsigned int sec_int_set_cloud_hash(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

SEC_RTN unsigned int sec_int_get_cloud_hash(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

SEC_RTN unsigned int sec_int_set_cloud_hash_dec(
	SEC_OUT void *p_sec_data,
	SEC_OUT unsigned int *p_sec_len
	);

SEC_RTN unsigned int sec_hc_int_hash_check(void);

/* ========================= IOSF SIDEBAND STUB =========================== */

SEC_RTN unsigned int sec_int_open_iosf_sb(
	SEC_INOUT struct sec_iosf_sb_context     *p_ctx
	);

SEC_RTN unsigned int sec_int_check_iosf_sb_ready(
	SEC_IN struct sec_iosf_sb_context     *p_ctx
	);

SEC_RTN unsigned int sec_int_read_iosf_sb(
	SEC_IN struct sec_iosf_sb_context     *p_ctx,
	SEC_IN unsigned int port,
	SEC_IN unsigned int addr,
	SEC_INOUT void     *p_sec_data,
	SEC_INOUT unsigned int     *p_sec_len,
	SEC_IN unsigned int is_blocking,
	SEC_IN unsigned int blocking_timeout,
	SEC_IN unsigned int (*p_non_blocking_callback)(void *)
	);

SEC_RTN unsigned int sec_int_close_iosf_sb(
	SEC_IN struct sec_iosf_sb_context     *p_ctx
	);

/* ========================= IOSF SIDEBAND STUB END ======================= */


/* ========================= Reprovison ============================= */

SEC_RTN unsigned int sec_int_set_reprovision(
	SEC_IN void     *p_sec_data,
	SEC_IN unsigned int sec_len,
	SEC_IN enum sec_info_type type
	);


/* ========================= NONCE Challenge ============================= */

SEC_RTN unsigned int sec_int_get_nonce(
	SEC_INOUT unsigned int     *p_nonce_data,
	SEC_IN unsigned int tran_id
	);

SEC_RTN unsigned int sec_int_check_nonce(
	SEC_IN unsigned char     *p_encrypted_nonce_data,
	SEC_IN unsigned int encrypted_nonce_data_len,
	SEC_IN unsigned char     *p_encrypted_nonce_iv,
	SEC_IN int encrypted_nonce_iv_len,
	SEC_IN unsigned char     *p_encrypted_nonce_tag,
	SEC_IN unsigned int encrypted_nonce_tag_len,
	SEC_IN unsigned int tran_id
	);

/* ========================= Handshake with BIOS =========================== */

SEC_RTN unsigned int sec_int_start(
	SEC_IN enum sec_mode mode,
	SEC_IN unsigned int timeout
	);

SEC_RTN unsigned int sec_int_get_rand_num(
	SEC_INOUT unsigned int     *p_rand_num_data,
	SEC_IN unsigned int size_in_bit
	);

SEC_RTN unsigned int sec_int_get_rand_num_by_mbed(
	SEC_INOUT unsigned int     *p_rand_num_data,
	SEC_IN unsigned int size_in_bit
	);

SEC_RTN unsigned int sec_int_lock_ctx_mutex(void);

SEC_RTN unsigned int sec_int_unlock_ctx_mutex(void);

SEC_RTN unsigned int sec_int_wait_timeout_async(
	SEC_IN unsigned int ms,
	SEC_IN EXPIRY_FUNC expiry_func
	);

SEC_RTN unsigned int sec_int_wait_timeout_sync(
	SEC_IN unsigned int ms
	);

SEC_RTN unsigned int sec_int_get_time(
	SEC_IN unsigned long *time_s
	);

SEC_RTN int mbedtls_hardware_poll(
	void *data,
	unsigned char *output,
	size_t len,
	size_t *olen
	);

SEC_RTN bool sec_hc_get_entropy_state(void);

SEC_RTN void sec_int_sec_decommission(void);

/* Use Set PM control from ehl-oob */
void oob_set_pm_control(bool set_oob);

#if defined(CONFIG_SOC_INTEL_PSE)
APP_GLOBAL_VAR(2) extern struct sec_context_wrapper sec_ctx;
APP_GLOBAL_VAR(2) extern unsigned int sec_int_start_init_flag;
#else
extern struct sec_context_wrapper sec_ctx;
extern unsigned int sec_int_start_init_flag;
#endif

#endif /* PSE_OOB_SEC_INTERNAL_H */

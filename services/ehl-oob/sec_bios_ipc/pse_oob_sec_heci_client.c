/**
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbedtls/sha256.h"     /* SHA-256 only */
#include "mbedtls/md.h"         /* generic interface */
#include "mbedtls/gcm.h"        /* mbedtls_gcm_context */
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/hkdf.h"

#include <device.h>
#include <init.h>
#include <string.h>
#include <stdlib.h>
#include <sys/printk.h>
#include <logging/log.h>

#include "heci.h"
#include "pse_oob_sec_base.h"
#include "pse_oob_sec_internal.h"
#include "pse_oob_sec_enum.h"
#include "pse_oob_sec_heci_client.h"
#include "pse_oob_sec_heci_client_internal.h"
#include "common/credentials.h"

LOG_MODULE_REGISTER(OOB_SEC_HC, CONFIG_OOB_LOGGING);

K_HEAP_DEFINE(oob_sec_hc_heap, SEC_HC_HEAP_SIZE);

/* sec heci thread id */
k_tid_t sec_hc_thread_id;

/* thread sem to sync between OOB thread and sec heci thread */
K_SEM_DEFINE(sec_hc_oob_sem, 0, 1);

static heci_rx_msg_t sec_hc_rx_msg;
static uint8_t sec_hc_rx_buffer[
	SEC_HC_MAX_RX_SIZE];

#define HECI_COMPOSE_BUFFER_LEN 400
/* The max length of a HECI message */
#define HECI_MESSAGE_THRESHOLD 4096

#if (defined(SEC_DEBUG) && SEC_DEBUG)
static uint8_t sec_hc_tx_buffer[
	SEC_HC_MAX_RX_SIZE] = {
	/* Dummy initialized value to ease debug */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
};
#else
static uint8_t sec_hc_tx_buffer[
	SEC_HC_MAX_RX_SIZE];
#endif

static K_THREAD_STACK_DEFINE(sec_hc_stack,
			     SEC_HC_STACK_SIZE);
static struct k_thread sec_hc_thread;
static K_SEM_DEFINE(sec_hc_event_sem, 0, 1);

static uint32_t sec_hc_event;
static uint32_t sec_hc_conn_id;
static uint32_t sec_hc_prev_command_id;
static bool sec_hc_abort_set;

typedef struct {
	mbedtls_gcm_context gcm;
} SEC_HC_CIPHER_T;
SEC_HC_CIPHER_T sec_hc_cipher_t;

typedef struct {
	/* Packet variable*/
	struct sec_hc_cmd_hdr_t *rxhdr;
	struct sec_hc_cmd_hdr_t *txhdr;
	union sec_hc_cmd_data_hdr_u *req_data;
} SEC_HC_PACKET_DATA_T;

typedef struct {
	/* Composing response packet */
	uint32_t *buf_compose;
	uint32_t *buf_compose_next;
	uint32_t accum_compose_len;
} SEC_HC_RESPONSE_PKT_T;
SEC_HC_RESPONSE_PKT_T sec_hc_resp_pkt;

/* Default CSE SEED are 10,
 * 4 is the max allowed seeds Now
 */
#define MAX_CSE_SEED_NO 4
typedef struct {
	unsigned char *a_cse_seed[MAX_CSE_SEED_NO];
	unsigned int a_cse_seed_len;
	unsigned int a_cse_seed_no;
} SEC_HC_CSE_SEED;
SEC_HC_CSE_SEED sec_hc_cse_seed;

static bool sec_hc_seed_updated;
/* ================================================================= */

static void sec_hc_print_context_param(
	uint8_t *text,
	uint8_t *p_ctx_param_data,
	uint32_t p_ctx_param_len
	)
{
	LOG_INF("p_ctx_param_len: %d\n", p_ctx_param_len);
	LOG_INF("%s: [Hex]\n", text);
	if (p_ctx_param_len > HECI_MESSAGE_THRESHOLD) {
		p_ctx_param_len = HECI_MESSAGE_THRESHOLD;
	}

	/* Enable this when more logs needed */
	for (int i = 0; i < p_ctx_param_len; i++) {
		printk("%02x ", (unsigned int)(*(p_ctx_param_data + i)));
	}

	LOG_INF("\n");
}

/* ================================================================= */
void sec_hc_final_decommission(void)
{
	LOG_INF("OOB Service is Waiting on Cloud event, Cancel the wait\n");
	/* sync OOB service thread and sec heci thread */
	k_sem_give(&sec_hc_oob_sem);

	/* Clear all OOB Data from Sec ctx */
	sec_int_sec_decommission();
	sec_hc_abort_set = true;
}

static void sec_hc_rel_cse_seed(void)
{
	LOG_INF("No of Seeds to be Released: %d\n",
		sec_hc_cse_seed.a_cse_seed_no);
	for (int seed_idx = 0; seed_idx < sec_hc_cse_seed.a_cse_seed_no;
	     seed_idx++) {
		k_heap_free(&oob_sec_hc_heap, sec_hc_cse_seed.a_cse_seed[seed_idx]);
		sec_hc_cse_seed.a_cse_seed[seed_idx] = NULL;
		LOG_INF("Free seed index: %d\n", seed_idx);
	}
	sec_hc_cse_seed.a_cse_seed_no = 0;
}

int sec_hc_process_cse_seed(void *p_sec_data, int idx, int data_len)
{
	LOG_INF("%s: get SEC_HC_TYPE_PSE_SEED_%d data\n", __func__, idx + 1);
	if (data_len > SEC_CSE_SEED_LEN) {
		LOG_DBG("[%s:%d] Invalid CSE Seed Length!: %d\n",
			__func__, __LINE__, data_len);
		return SEC_FAILED;
	}
	if (sec_hc_cse_seed.a_cse_seed[idx] != NULL) {
		LOG_DBG("[%s:%d] Same Seed_%d Sent From BIOS Check BIOS CMD!\n",
			__func__, __LINE__, idx);
		return SEC_FAILED;
	}
	sec_hc_cse_seed.a_cse_seed[idx] = (unsigned char *)k_heap_alloc(&oob_sec_hc_heap,
							SEC_CSE_SEED_LEN * sizeof(char),
							K_NO_WAIT);

	if (sec_hc_cse_seed.a_cse_seed[idx] == NULL) {
		LOG_DBG("[%s:%d] Memory Allocation Failed!\n",
			__func__, __LINE__);
		return SEC_FAILED;
	}
	memcpy(sec_hc_cse_seed.a_cse_seed[idx], p_sec_data, SEC_CSE_SEED_LEN);
	sec_hc_cse_seed.a_cse_seed_no = sec_hc_cse_seed.a_cse_seed_no + 1;
	sec_hc_print_context_param("sec_hc_cse_seed.a_cse_seed[idx]",
				   (uint8_t *)sec_hc_cse_seed.a_cse_seed[idx],
				   SEC_CSE_SEED_LEN);

	return SEC_SUCCESS;
}

int sec_hc_process_cmd_data_once(union sec_hc_cmd_data_hdr_u *cmd_hdr_data,
	struct sec_context *ctx,
	uint32_t conti
	)
{
	int result = SEC_SUCCESS;
	union sec_hc_cmd_data_hdr_u *hdr = cmd_hdr_data;
	sec_hc_cmd_data_t *data =
		(sec_hc_cmd_data_t *)(cmd_hdr_data + 1);

	int data_len = 0;

	data_len = (hdr->bitfields.length)
		   - sizeof(union sec_hc_cmd_data_hdr_u);

	if ((data_len <= 0)) {
		result = SEC_FAILED;
		return result;
	}

	LOG_INF("[[\n");
	LOG_INF("(hdr->whole) = 0x%08x 0d%d\n",
		(unsigned int)(hdr->whole), (unsigned int)(hdr->whole));
	LOG_INF(" (hdr->bitfields.type) = 0x%02x 0d%d\n",
		(hdr->bitfields.type), (hdr->bitfields.type));
	LOG_INF(" (hdr->bitfields.length) = 0x%03x 0d%d\n",
		(hdr->bitfields.length), (hdr->bitfields.length));
	LOG_INF(" payload size = 0x%03x 0d%d\n", data_len, data_len);

	sec_hc_print_context_param("get data",
				   (uint8_t *)(cmd_hdr_data), data_len);

	switch ((hdr->bitfields.type)) {

	case SEC_HC_TYPE_DENC_MQTT_CLIENT_ID:
		LOG_INF("get SEC_HC_TYPE_MQTT_CLIENT_ID data\n");
		sec_int_set_mqtt_client_id((void *)(data), &data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_MQTT_CLIENT_ID data",
			(ctx->a_mqtt_client_id), (ctx->a_mqtt_client_id_len));
		break;

	case SEC_HC_TYPE_ENC_MQTT_CLIENT_ID:
		LOG_INF("get SEC_HC_TYPE_ENC_MQTT_CLIENT_ID data\n");
		if (data_len > SEC_MQTT_CLIENT_ID_LEN) {
			LOG_DBG("[%s:%d] Invalid ENC MQTT Client ID Len! :%d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;
		}
		memmove(sec_ctx.a_enc_mqtt_client_id, data, data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_ENC_MQTT_CLIENT_ID data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_ENC_MQTT_CLIENT_ID_TAG:
		LOG_INF("get SEC_HC_TYPE_ENC_MQTT_CLIENT_ID_TAG data\n");
		if (data_len > SEC_ENC_TAG_SIZE) {
			LOG_DBG("[%s:%d]Invalid ENC MQTT Client TAG Len!: %d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;
		}
		memmove(sec_ctx.a_enc_mqtt_client_id_tag, data, data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_ENC_MQTT_CLIENT_ID_TAG data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_ENC_MQTT_CLIENT_ID_IV:
		LOG_INF("get SEC_HC_TYPE_ENC_MQTT_CLIENT_ID_IV data\n");
		if (data_len > SEC_ENC_IV_SIZE) {
			LOG_DBG("[%s:%d] Invalid ENC MQTT Client IV Len!: %d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;
		}
		memmove(sec_ctx.a_enc_mqtt_client_id_iv, data, data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_ENC_MQTT_CLIENT_ID_IV data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_ROOTCA:
		LOG_INF("get SEC_HC_TYPE_ROOTCA data\n");
		sec_int_set_root_ca((void *)(data), &data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_ROOTCA data",
			(ctx->a_root_ca), (ctx->root_ca_len));
		break;

	case SEC_HC_TYPE_256_OWN_PUB_KEY:
		LOG_INF("get SEC_HC_TYPE_256_OWN_PUB_KEY data\n");
		sec_int_set_own_pub_key((void *)(data), &data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_256_OWN_PUB_KEY data",
			(ctx->a_own_pub_key),
			(ctx->own_pub_key_len));

		break;

	case SEC_HC_TYPE_HKDF_32B_PSE_SALT:
		LOG_INF("get SEC_HC_TYPE_HKDF_32B_PSE_SALT data\n");
		sec_int_set_hkdf_32b_pse_salt((void *)(data), &data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_HKDF_32B_PSE_SALT data",
			(ctx->a_hkdf_32b_pse_salt),
			(ctx->hkdf_32b_pse_salt_len));
		break;

	case SEC_HC_TYPE_DENC_TOK_ID:
		LOG_INF("get SEC_HC_TYPE_DENC_TKN_ID data\n");
		sec_int_set_tok_id((void *)(data), &data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_DENC_TOK_ID data",
			(ctx->a_tok_id), (ctx->tok_id_len));
		break;

	case SEC_HC_TYPE_DENC_DEV_ID:
		LOG_INF("get SEC_HC_TYPE_DENC_DEV_ID data\n");
		sec_int_set_dev_id((void *)(data), &data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_DENC_DEV_ID data",
			(ctx->a_dev_id), (ctx->dev_id_len));
		break;

	case SEC_HC_TYPE_ENC_TOK_ID:
		LOG_INF("get SEC_HC_TYPE_ENC_TKN_ID data\n");
		if (data_len > SEC_TOK_ID_LEN) {
			LOG_DBG("[%s:%d] Invalid ENC Token ID Len! :%d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;
		}
		memmove(sec_ctx.a_enc_tok_id, data, data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_ENC_TOK_ID data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_ENC_TOK_ID_TAG:
		LOG_INF("get SEC_HC_TYPE_ENC_TOK_ID_TAG data\n");
		if (data_len > SEC_ENC_TAG_SIZE) {
			LOG_DBG("[%s:%d] Invalid ENC Token TAG Len! :%d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;
		}
		memmove(sec_ctx.a_enc_tok_id_tag, data, data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_ENC_TOK_ID_TAG data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_ENC_TOK_ID_IV:
		LOG_INF("get SEC_HC_TYPE_ENC_TOK_ID_IV data\n");
		if (data_len > SEC_ENC_IV_SIZE) {
			LOG_DBG("[%s:%d] Invalid ENC Token IV Len! :%d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;
		}
		memmove(sec_ctx.a_enc_tok_id_iv, data, data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_ENC_TOK_ID_IV data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_ENC_DEV_ID:
		LOG_INF("get SEC_HC_TYPE_ENC_DVCE_ID data\n");
		if (data_len > SEC_DEV_ID_LEN) {
			LOG_DBG("[%s:%d] Invalid ENC Device ID Len! :%d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;
		}
		memmove(sec_ctx.a_enc_dev_id, data, data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_ENC_DEV_ID data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_ENC_DEV_ID_TAG:
		LOG_INF("get TYPE_ENC_DEV_ID_TAG data\n");
		/* Need Fix from BIOS: Uncomment after BIOS changes.
		 * if (data_len > SEC_ENC_TAG_SIZE) {
		 *	LOG_DBG("[%s:%d] Invalid ENC Device TAG Len! :%d\n",
		 *		__func__, __LINE__, data_len);
		 *	result = SEC_FAILED;
		 *	break;
		 * }
		 */
		memmove(sec_ctx.a_enc_dev_id_tag, data, data_len);
		sec_hc_print_context_param(
			"get TYPE_ENC_DEV_ID_TAG data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_ENC_DEV_ID_IV:
		LOG_INF("get TYPE_ENC_DEV_ID_IV data\n");
		if (data_len > SEC_ENC_IV_SIZE) {
			LOG_DBG("[%s:%d] Invalid ENC Device IV Len! :%d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;
		}
		memmove(sec_ctx.a_enc_dev_id_iv, data, data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_ENC_DEV_ID_IV data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_CLD_URL:
		LOG_INF("get SEC_HC_TYPE_CLD_URL data\n");
		sec_int_set_cld_host_url((void *)(data), &data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_CLD_URL data",
			(uint8_t *)(ctx->a_cld_host_url),
			ctx->cld_host_url_len);
		break;

	case SEC_HC_TYPE_CLD_PORT:
		LOG_INF("get SEC_HC_TYPE_CLD_PORT data\n");
		sec_int_set_cld_host_port((void *)(data));
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_CLD_PORT data",
			(uint8_t *)&(ctx->cld_host_port), 4);
		break;

	case SEC_HC_TYPE_PXY_URL:
		LOG_INF("get SEC_HC_TYPE_PXY_URL data\n");
		sec_int_set_pxy_host_url((void *)(data), &data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_PXY_URL data",
			(uint8_t *)(ctx->a_pxy_host_url),
			ctx->pxy_host_url_len);
		break;

	case SEC_HC_TYPE_PXY_PORT:
		LOG_INF("get SEC_HC_TYPE_PXY_PORT data\n");
		sec_int_set_pxy_host_port((void *)(data));
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_PXY_PORT data",
			(uint8_t *)&(ctx->pxy_host_port), 4);
		break;

	case SEC_HC_TYPE_CLD_ADAPTER:
		LOG_INF("get SEC_HC_TYPE_CLD_ADAPTER data\n");
		sec_int_set_cld_adapter((void *)(data), &data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_CLD_ADAPTER data",
			(uint8_t *)(ctx->a_cld_adapter),
			ctx->cld_adapter_len);
		break;

	case SEC_HC_TYPE_PROV_STATE:
		LOG_INF("get SEC_HC_TYPE_PROV_STATE data\n");
		sec_int_set_prov_state((void *)(data));
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_PROV_STATE data",
			(uint8_t *)&(ctx->prov_state), 4);
		break;

	case SEC_HC_TYPE_DENC_CLOUD_HASH_ID:
		LOG_INF("get SEC_HC_TYPE_DENC_CLOUD_HASH_ID data\n");
		sec_int_set_cloud_hash((void *) (data), &data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_CLD_ADAPTER data",
			(uint8_t *)(ctx->a_cloud_hash),
			ctx->a_cloud_hash_len);
		break;

	case SEC_HC_TYPE_ENC_CLOUD_HASH_ID:
		LOG_INF("get SEC_HC_TYPE_ENC_CLOUD_HASH_ID data\n");
		if (data_len > SEC_CLOUD_HASH_LEN) {
			LOG_DBG("[%s:%d] Invalid ENC Cloud hash ID Len!: %d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;
		}
		memmove(sec_ctx.a_enc_cloud_hash_id, data, data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_ENC_CLOUD_HASH_ID data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_ENC_CLOUD_HASH_ID_TAG:
		LOG_INF("get SEC_HC_TYPE_ENC_CLOUD_HASH_ID_TAG data\n");
		if (data_len > SEC_ENC_TAG_SIZE) {
			LOG_DBG("[%s:%d] Invalid ENC Cloud hash TAG Len!: %d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;
		}
		memmove(sec_ctx.a_enc_cloud_hash_id_tag, data, data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_END_CLOUD_HASH_ID_TAG data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_ENC_CLOUD_HASH_ID_IV:
		LOG_INF("get SEC_HC_TYPE_ENC_CLOUD_HASH_ID_IV data\n");
		if (data_len > SEC_ENC_IV_SIZE) {
			LOG_DBG("[%s:%d] Invalid ENC Cloud hash IV Len!: %d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;
		}
		memmove(sec_ctx.a_enc_cloud_hash_id_iv, data, data_len);
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_END_CLOUD_HASH_ID_IV data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_PSE_SEED_1:
		if (SEC_SUCCESS !=
		    sec_hc_process_cse_seed(data, CSE_SEED1, data_len)) {
			LOG_DBG("[%s:%d] CSE Seed1 process Failed!: %d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;
		}
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_PSE_SEED_1 data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_PSE_SEED_2:
		if (SEC_SUCCESS !=
		    sec_hc_process_cse_seed(data, CSE_SEED2, data_len)) {
			LOG_DBG("[%s:%d] CSE Seed2 process Failed!: %d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;
		}
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_PSE_SEED_2 data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_PSE_SEED_3:
		if (SEC_SUCCESS !=
		    sec_hc_process_cse_seed(data, CSE_SEED3, data_len)) {
			LOG_DBG("[%s:%d] CSE Seed3 process Failed!: %d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;

		}
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_PSE_SEED_3 data",
			((uint8_t  *)data), data_len);
		break;

	case SEC_HC_TYPE_PSE_SEED_4:
		if (SEC_SUCCESS !=
		    sec_hc_process_cse_seed(data, CSE_SEED4, data_len)) {
			LOG_DBG("[%s:%d] CSE Seed3 process Failed!: %d\n",
				__func__, __LINE__, data_len);
			result = SEC_FAILED;
			break;
		}
		sec_hc_print_context_param(
			"get SEC_HC_TYPE_PSE_SEED_4 data",
			((uint8_t  *)data), data_len);
		break;

	default:
		LOG_INF("get UNSUPPORTED data\n");
		result = SEC_FAILED;
		break;
	}

	LOG_INF("]]\n");
	return result;

}

int sec_hc_process_cmd_data(union sec_hc_cmd_data_hdr_u *req_data,
	int req_len,
	struct sec_context *ctx,
	uint32_t conti
	)
{
	int result = SEC_SUCCESS;
	union sec_hc_cmd_data_hdr_u *hdr = req_data;
	int len = req_len;
	uint8_t *mid_addr;

	LOG_INF("len =%d\n", len);
	if (len <= 0) {
		result = SEC_FAILED;
		return result;
	}

	while (len > 0) {
		LOG_INF("[\n\n");
		result = sec_hc_process_cmd_data_once(
			hdr, ctx, conti);

		len = len - (hdr->bitfields.length);

		/* Pointer to next data type structure do last */
		mid_addr = (uint8_t *) hdr;
		mid_addr = mid_addr + (hdr->bitfields.length);
		hdr = (union sec_hc_cmd_data_hdr_u *) mid_addr;

		LOG_INF("len =%d\n", len);
		LOG_INF("]\n\n");
	}

	return result;
}

int sec_hc_create_cmd_data(
	uint32_t *p_compose_buf,
	uint32_t compose_len,
	uint32_t **p_new_compose_buf,
	uint32_t *p_new_accum_len,
	union sec_hc_cmd_data_hdr_u *hdr,
	uint32_t *data
	)
{
	int result = SEC_SUCCESS;
	uint8_t *mid_addr = 0;

	if ((compose_len - (hdr->bitfields.length)) < 0) {
		result = SEC_FAILED;
		return result;
	}

	LOG_INF("[%s][[\n", __func__);
	LOG_INF("(hdr->whole) = 0x%08x 0d%d\n",
		(unsigned int)(hdr->whole), (unsigned int)(hdr->whole));
	LOG_INF(" (hdr->bitfields.type) = 0x%02x\n", (hdr->bitfields.type));
	LOG_INF(" (hdr->bitfields.length) = 0x%03x 0d%d\n",
		(hdr->bitfields.length), (hdr->bitfields.length));

	memmove(p_compose_buf, hdr,
		sizeof(union sec_hc_cmd_data_hdr_u));

	mid_addr = (uint8_t *) p_compose_buf;
	LOG_INF(" (start buffer address) = 0x%08x\n",
		(unsigned int)(mid_addr));

	mid_addr = mid_addr
		   + sizeof(union sec_hc_cmd_data_hdr_u);
	LOG_INF(" (+hdr buffer address) = 0x%08x\n",
		(unsigned int)(mid_addr));

	memmove(
		mid_addr,
		data,
		((hdr->bitfields.length) -
		 sizeof(union sec_hc_cmd_data_hdr_u)));

	mid_addr = (uint8_t *) p_compose_buf + (hdr->bitfields.length);
	*p_new_compose_buf = (uint32_t *) mid_addr;
	*p_new_accum_len = *p_new_accum_len + hdr->bitfields.length;

	LOG_INF("]]\n");
	return result;
}

static int  sec_hc_gen_random_num(unsigned char *buf, int len)
{
	int ret;
	char *personalization = "SEC_INTERNAL";
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_entropy_context entropy;

	mbedtls_ctr_drbg_init(&ctr_drbg);
	mbedtls_entropy_init(&entropy);

	ret = mbedtls_ctr_drbg_seed(&ctr_drbg,
				    mbedtls_entropy_func,
				    &entropy,
				    (const unsigned char *)personalization,
				    strlen(personalization));
	if (ret != 0) {
		LOG_INF("mbedtls dnrg seed failed: %d\n", ret);
		SEC_ASSERT(ret);
	}

	ret = mbedtls_ctr_drbg_random(&ctr_drbg, buf, len);
	if (ret != 0) {
		LOG_INF("mbedtls drng random gen failed!\n");
		SEC_ASSERT(ret);
	}
	LOG_INF("\n");
	sec_hc_print_context_param("Random Num output: ",
				   (uint8_t *)buf, len);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
	return ret;
}

static unsigned int sec_hc_resp_data_int(SEC_HC_PACKET_DATA_T *sec_hc_pkt)
{
	sec_hc_resp_pkt.buf_compose =
		(uint32_t  *)(sec_hc_pkt->txhdr + 1);
	sec_hc_resp_pkt.accum_compose_len = 4;
	sec_hc_resp_pkt.buf_compose_next = NULL;

	return SEC_SUCCESS;
}

static SEC_HC_PACKET_DATA_T *sec_hc_setup_rxhdr(uint8_t *buf)
{
	SEC_HC_PACKET_DATA_T *sec_hc_pkt;

	sec_hc_pkt = (SEC_HC_PACKET_DATA_T *)k_malloc(
		sizeof(SEC_HC_PACKET_DATA_T));

	if (sec_hc_pkt == NULL) {
		LOG_DBG("[%s:%d] Memory Allocation Fail! len = %u\n",
			__func__, __LINE__, sizeof(SEC_HC_PACKET_DATA_T));
		return NULL;
	}

	sec_hc_pkt->rxhdr = (struct sec_hc_cmd_hdr_t *)buf;
	sec_hc_pkt->req_data =
		(union sec_hc_cmd_data_hdr_u *)(sec_hc_pkt->rxhdr + 1);
	sec_hc_pkt->txhdr = (struct sec_hc_cmd_hdr_t *) sec_hc_tx_buffer;

	LOG_INF("(rxhdr->msg_comp) :0x%08xs\n", (sec_hc_pkt->rxhdr->msg_comp));
	LOG_INF("(rxhdr->length) :0x%08x\n", (sec_hc_pkt->rxhdr->length));
	LOG_INF("(rxhdr->command) :0x%08x\n", (sec_hc_pkt->rxhdr->command));
	sec_hc_print_context_param("[get ipc buf data]", (uint8_t *)(buf),
				   (sec_hc_pkt->rxhdr->length));
	return sec_hc_pkt;
}

static void sec_hc_txhdr_config(SEC_HC_PACKET_DATA_T *sec_hc_pkt)
{
	sec_hc_pkt->txhdr->msg_comp = 1;
	sec_hc_pkt->txhdr->rsvd = 0;
	sec_hc_pkt->txhdr->status = SEC_HC_STATUS_OK;
	sec_hc_pkt->txhdr->protocol_ver = SEC_HC_MAJOR_MINOR_VERSION;
	sec_hc_pkt->txhdr->source = SEC_HC_SOURCE_OOB;

	if ((sec_hc_pkt->rxhdr->rsvd) != 0) {
		LOG_INF("[sec_heci] (rxhdr->rsvd)!= 0\n");
		sec_hc_pkt->txhdr->status = SEC_HC_STATUS_ERROR;
	}
	if ((sec_hc_pkt->rxhdr->protocol_ver) <
	    SEC_HC_MAJOR_MINOR_VERSION) {
		LOG_INF("[sec_heci]protocol_version < VERSION\n");
		sec_hc_pkt->txhdr->status = SEC_HC_STATUS_ERROR;
	}
	if ((sec_hc_pkt->rxhdr->source) != SEC_HC_SOURCE_BIOS) {
		LOG_INF("[sec_heci] source != SOURCE_BIOS\n");
		sec_hc_pkt->txhdr->status = SEC_HC_STATUS_ERROR;
	}
	if ((sec_hc_pkt->rxhdr->length) < 3) {
		LOG_INF("[sec_heci] (rxhdr->length)< 3\n");
		sec_hc_pkt->txhdr->status = SEC_HC_STATUS_ERROR;
	}
}

int sec_hc_hkdf_key_gen(unsigned char *type,
	int type_len,
	unsigned char *a_id_enc_key
	)
{
	int ret = SEC_SUCCESS;
	int size;
	unsigned char buffer_pse_salt[SEC_PSE_SALT_LEN];
	unsigned char pse_32b_fuse[SEC_FUSE_LEN];

	mbedtls_md_type_t md_type;
	const mbedtls_md_info_t *p_md_info;

	md_type = MBEDTLS_MD_SHA256;
	p_md_info = mbedtls_md_info_from_type(md_type);

	size = SEC_FUSE_LEN;
	memset(pse_32b_fuse, 0x00, SEC_FUSE_LEN);
	ret = sec_int_get_pse_32b_fuse(pse_32b_fuse, &size);
	if (ret != SEC_SUCCESS) {
		LOG_INF("HKDF keys generation: Fail to get PSE Fuse\n");
		return ret;
	}
	sec_hc_print_context_param(
		"HKDF keys generation: pse_32b_fuse ",
		(uint8_t *)pse_32b_fuse, SEC_FUSE_LEN);

	size = SEC_PSE_SALT_LEN;
	memset(buffer_pse_salt, 0x00, SEC_PSE_SALT_LEN);
	sec_int_get_hkdf_32b_pse_salt(buffer_pse_salt, &size);
	sec_hc_print_context_param(
		"HKDF keys generation: buffer_pse_salt ",
		(uint8_t *)buffer_pse_salt, SEC_PSE_SALT_LEN);

	/* HKDF keys generation: */
	ret = mbedtls_hkdf(
		p_md_info,
		buffer_pse_salt,
		SEC_PSE_SALT_LEN,
		pse_32b_fuse,
		SEC_FUSE_LEN,
		type,
		type_len,
		a_id_enc_key, /* Key will be stored */
		SEC_FUSE_LEN
		);
	if (ret != SEC_SUCCESS) {
		LOG_INF("HKDF keys generation: Fail for: %s, ret: %d\n",
			type, ret);
	} else {
		sec_hc_print_context_param(
			type, (uint8_t *)a_id_enc_key, SEC_FUSE_LEN);
	}
	return ret;
}

int sec_hc_encrypt_data_once(
	unsigned char *key,
	unsigned char *clear_text,
	unsigned int clear_text_len,
	unsigned char *encrypted,
	unsigned char *a_encrypted_iv,
	unsigned char *a_encrypted_tag
	)
{

	int ret = SEC_SUCCESS;
	unsigned int key_len = SEC_FUSE_LEN;

	/* TRNG hardware generated seed in MBEDTLS */
	if (sec_hc_get_entropy_state()) {
		ret = sec_hc_gen_random_num(
			(unsigned char *)a_encrypted_iv,
			SEC_ENC_IV_SIZE);
		if (ret == SEC_SUCCESS) {
			sec_hc_print_context_param(
				"a_encrypted_iv: ",
				(uint8_t *)a_encrypted_iv,
				SEC_ENC_IV_SIZE);
		}
	} else {
		memset(a_encrypted_iv, 0x00, SEC_ENC_IV_SIZE);
	}

	ret = mbedtls_gcm_setkey(
		&sec_hc_cipher_t.gcm,
		MBEDTLS_CIPHER_ID_AES,
		key,
		key_len * 8
		);

	if (ret != 0) {
		LOG_INF("mbedtls_gcm_setkey() failed for: %s, ret: %d\n",
			clear_text, ret);
		return SEC_FAILED;
	}

	memset((void *)encrypted, 0, clear_text_len);
	memset((void *)a_encrypted_tag, 0, SEC_ENC_TAG_SIZE);

	ret = mbedtls_gcm_crypt_and_tag(
		&sec_hc_cipher_t.gcm,
		MBEDTLS_GCM_ENCRYPT,
		clear_text_len,
		a_encrypted_iv,
		SEC_ENC_IV_SIZE,
		(unsigned char *)"",
		strlen(""),
		clear_text,
		encrypted,
		SEC_ENC_TAG_SIZE,
		a_encrypted_tag);

	if (ret != 0) {
		LOG_INF("mbedtls_gcm_crypt_and_tag() failed for: %s, ret: %d\n",
			clear_text, ret);
		return SEC_FAILED;
	}
	sec_hc_print_context_param(
		"encrypted: ", (uint8_t *)encrypted, clear_text_len);
	sec_hc_print_context_param(
		"a_encrypted_tag: ", (uint8_t *)a_encrypted_tag, SEC_ENC_TAG_SIZE);
	return ret;
}

int sec_hc_decrypt_data_once(
	unsigned char *key,
	unsigned char *clear_text,
	unsigned int clear_text_len,
	unsigned char *encrypted,
	unsigned char *a_encrypted_iv,
	unsigned char *a_encrypted_tag
	)
{
	int ret = SEC_SUCCESS;
	unsigned int key_len = SEC_FUSE_LEN;

	LOG_INF(" 5 key_len: %d\n", key_len);
	LOG_INF("key_len * 8 : %d\n", key_len * 8);

	ret = mbedtls_gcm_setkey(
		&sec_hc_cipher_t.gcm,
		MBEDTLS_CIPHER_ID_AES,
		key,
		key_len * 8
		);

	if (ret != 0) {
		LOG_INF("mbedtls_gcm_setkey() failed for: %s, ret: %d\n",
			clear_text, ret);
		return SEC_FAILED;
	}

	memset((void *)clear_text, 0, clear_text_len);
	ret = mbedtls_gcm_auth_decrypt(
		&sec_hc_cipher_t.gcm,
		clear_text_len,
		a_encrypted_iv,
		SEC_ENC_IV_SIZE,
		(unsigned char *)"",
		0,
		a_encrypted_tag,
		SEC_ENC_TAG_SIZE,
		encrypted,
		clear_text);

	if (ret != 0) {
		LOG_INF("mbedtls_gcm_auth_decrypt() failed for: %s, ret: %d\n",
			clear_text, ret);
		return SEC_FAILED;
	}
	sec_hc_print_context_param("Decrypted Data: ",
				   (uint8_t *)clear_text, clear_text_len);
	return ret;
}

static int sec_hc_encrypt_data(
	sec_hc_type_enc_a data_type,
	unsigned int buf_len)
{
	int size;
	int ret = SEC_SUCCESS;
	unsigned char *buffer = (unsigned char *)k_heap_alloc(&oob_sec_hc_heap, buf_len, K_NO_WAIT);

	if (buffer == NULL) {
		LOG_DBG("[%s:%d] Memory Allocation Fail, buf_len = %d !\n",
			__func__, __LINE__, buf_len);
		return SEC_FAILED;
	}

	memset(buffer, '\0', buf_len);
	switch (data_type) {
	case SEC_HC_TYPE_ENC_TOK:
		/* MBED encryption: token id start */
		LOG_INF("Create token id key\n");
		ret = sec_hc_hkdf_key_gen(
			"TOK_ENC_KEY",
			strlen("TOK_ENC_KEY"),
			sec_ctx.a_tok_id_enc_key
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		LOG_INF("encryption: token id generation\n");
		sec_int_get_tok_id(buffer, &size);
		ret = sec_hc_encrypt_data_once(
			sec_ctx.a_tok_id_enc_key,
			buffer,
			buf_len,
			sec_ctx.a_enc_tok_id,
			sec_ctx.a_enc_tok_id_iv,
			sec_ctx.a_enc_tok_id_tag
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		sec_hc_print_context_param("Enc token ID : ",
					   (uint8_t *)sec_ctx.a_enc_tok_id,
					   SEC_TOK_ID_LEN);
		break;
	/* MBED encryption: token id end */

	case SEC_HC_TYPE_ENC_DEV:
		/* MBED encryption: Device id start */
		LOG_INF("Create device id key\n");
		ret = sec_hc_hkdf_key_gen(
			"DEV_ENC_KEY",
			strlen("DEV_ENC_KEY"),
			sec_ctx.a_dev_id_enc_key
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		LOG_INF("encryption: device id generation\n");
		sec_int_get_dev_id(buffer, &size);
		ret = sec_hc_encrypt_data_once(
			sec_ctx.a_dev_id_enc_key,
			buffer,
			buf_len,
			sec_ctx.a_enc_dev_id,
			sec_ctx.a_enc_dev_id_iv,
			sec_ctx.a_enc_dev_id_tag
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		sec_hc_print_context_param(
			"Enc Device ID : ",
			(uint8_t *)sec_ctx.a_enc_dev_id, SEC_DEV_ID_LEN);
		break;
	/* MBED encryption: device id end */

	case SEC_HC_TYPE_ENC_MQTT_ID:
		/* MBED encryption: MQTT id start */
		LOG_INF("Create mqtt client id key\n");
		ret = sec_hc_hkdf_key_gen(
			"MQTT_ID_ENC_KEY",
			strlen("MQTT_ID_ENC_KEY"),
			sec_ctx.a_mqtt_client_id_enc_key
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		LOG_INF("encryption: mqtt client id\n");
		sec_int_get_mqtt_client_id(buffer, &size);
		ret = sec_hc_encrypt_data_once(
			sec_ctx.a_mqtt_client_id_enc_key,
			buffer,
			buf_len,
			sec_ctx.a_enc_mqtt_client_id,
			sec_ctx.a_enc_mqtt_client_id_iv,
			sec_ctx.a_enc_mqtt_client_id_tag
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		sec_hc_print_context_param(
			"Enc mqtt client ID : ",
			(uint8_t *)sec_ctx.a_enc_mqtt_client_id,
			SEC_MQTT_CLIENT_ID_LEN);
		break;
	/* MBED encryption: mqtt client id end */

	case SEC_HC_TYPE_ENC_CLD_HASH:
		/* MBED encryption: Cloud hash id start */
		LOG_INF("Create cld hash id key\n");
		ret = sec_hc_hkdf_key_gen(
			"CLD_HASH_ENC_KEY",
			strlen("CLD_HASH_ENC_KEY"),
			sec_ctx.a_cloud_hash_enc_key
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		LOG_INF("encryption: cld hash id generation\n");
		sec_int_get_cloud_hash(buffer, &size);
		ret = sec_hc_encrypt_data_once(
			sec_ctx.a_cloud_hash_enc_key,
			buffer,
			buf_len,
			sec_ctx.a_enc_cloud_hash_id,
			sec_ctx.a_enc_cloud_hash_id_iv,
			sec_ctx.a_enc_cloud_hash_id_tag
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		sec_hc_print_context_param(
			"Enc cld hash ID : ",
			(uint8_t *)sec_ctx.a_enc_cloud_hash_id,
			SEC_CLOUD_HASH_LEN);
		break;
	/* MBED encryption: cld hash id end */

	default:
		LOG_INF("Wrong Encryption data type\n");
		break;
	}

	k_heap_free(&oob_sec_hc_heap, buffer);
	return ret;
}

int sec_hc_decrypt_data(sec_hc_type_dec_a data_type, unsigned int buf_len)
{
	int ret = SEC_SUCCESS;
	unsigned char *buffer = (unsigned char *)k_heap_alloc(&oob_sec_hc_heap, buf_len, K_NO_WAIT);

	if (buffer == NULL) {
		LOG_DBG("[%s:%d] Memory Allocation Fail!\n",
			__func__, __LINE__);
		return SEC_FAILED;
	}

	memset(buffer, '\0', buf_len);
	switch (data_type) {
	case SEC_HC_TYPE_DEC_TOK:
		/* MBED decryption: token id start */
		LOG_INF("Decryption: Create token id key\n");
		ret = sec_hc_hkdf_key_gen(
			"TOK_ENC_KEY",
			strlen("TOK_ENC_KEY"),
			sec_ctx.a_tok_id_enc_key
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		sec_hc_print_context_param(
			"sec_ctx.a_tok_id_enc_key: ",
			(uint8_t *)sec_ctx.a_tok_id_enc_key,
			SEC_FUSE_LEN);

		LOG_INF("Decryption: token id\n");
		ret = sec_hc_decrypt_data_once(
			sec_ctx.a_tok_id_enc_key,
			buffer,
			buf_len,
			sec_ctx.a_enc_tok_id,
			sec_ctx.a_enc_tok_id_iv,
			sec_ctx.a_enc_tok_id_tag
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		sec_int_set_tok_id((void *)(buffer), &buf_len);
		sec_hc_print_context_param(
			"Decrypted token id: ",
			(uint8_t *)buffer,
			SEC_TOK_ID_LEN);

		LOG_INF("Decryption: token ID: %s\n", buffer);
		break;
	/* MBED decryption: token id End */

	case SEC_HC_TYPE_DEC_DEV:
		/* MBED decryption: Device id start */
		LOG_INF("Decryption: Create device id key\n");
		ret = sec_hc_hkdf_key_gen(
			"DEV_ENC_KEY",
			strlen("DEV_ENC_KEY"),
			sec_ctx.a_dev_id_enc_key
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		sec_hc_print_context_param(
			"sec_ctx.a_dev_id_enc_key: ",
			(uint8_t *)sec_ctx.a_dev_id_enc_key,
			SEC_FUSE_LEN);

		LOG_INF("Decryption: Device id\n");
		ret = sec_hc_decrypt_data_once(
			sec_ctx.a_dev_id_enc_key,
			buffer,
			buf_len,
			sec_ctx.a_enc_dev_id,
			sec_ctx.a_enc_dev_id_iv,
			sec_ctx.a_enc_dev_id_tag
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		sec_hc_print_context_param(
			"Decrypted device id: ",
			(uint8_t *)buffer,
			SEC_DEV_ID_LEN);
		sec_int_set_dev_id((void *)(buffer), &buf_len);

		LOG_INF("Decryption: Device ID: %s\n", buffer);
		break;
	/* MBED encryption: Device id end */

	case SEC_HC_TYPE_DEC_MQTT_ID:
		/* MBED decryption: MQTT id start */
		LOG_INF("Decryption: Create MQTT Client id key\n");
		ret = sec_hc_hkdf_key_gen(
			"MQTT_ID_ENC_KEY",
			strlen("MQTT_ID_ENC_KEY"),
			sec_ctx.a_mqtt_client_id_enc_key
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		sec_hc_print_context_param(
			"sec_ctx.a_mqtt_client_id_enc_key: ",
			(uint8_t *)sec_ctx.a_mqtt_client_id_enc_key,
			SEC_FUSE_LEN);

		/* MBED decryption: token id start */
		LOG_INF("Decryption: mqtt client id\n");
		ret = sec_hc_decrypt_data_once(
			sec_ctx.a_mqtt_client_id_enc_key,
			buffer,
			buf_len,
			sec_ctx.a_enc_mqtt_client_id,
			sec_ctx.a_enc_mqtt_client_id_iv,
			sec_ctx.a_enc_mqtt_client_id_tag
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		sec_hc_print_context_param(
			"Decrypted mqtt client id: ",
			(uint8_t *)buffer,
			SEC_MQTT_CLIENT_ID_LEN);
		sec_int_set_mqtt_client_id((void *)(buffer), &buf_len);

		LOG_INF("Decryption: mqtt client ID: %s\n", buffer);
		break;
	/* MBED decryption: MQTT id End */

	case SEC_HC_TYPE_DEC_CLD_HASH:
		/* MBED decryption: cloud hash id start */
		LOG_INF("Decryption: Create cld hash id key\n");
		ret = sec_hc_hkdf_key_gen(
			"CLD_HASH_ENC_KEY",
			strlen("CLD_HASH_ENC_KEY"),
			sec_ctx.a_cloud_hash_enc_key
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		sec_hc_print_context_param(
			"sec_ctx.a_cloud_hash_enc_key: ",
			(uint8_t *)sec_ctx.a_cloud_hash_enc_key,
			SEC_FUSE_LEN);

		LOG_INF("Decryption: cld hash id\n");
		ret = sec_hc_decrypt_data_once(
			sec_ctx.a_cloud_hash_enc_key,
			buffer,
			buf_len,
			sec_ctx.a_enc_cloud_hash_id,
			sec_ctx.a_enc_cloud_hash_id_iv,
			sec_ctx.a_enc_cloud_hash_id_tag
			);
		if (ret != SEC_SUCCESS) {
			break;
		}
		sec_hc_print_context_param(
			"Decrypted cld hash val: ",
			(uint8_t *)buffer,
			SEC_CLOUD_HASH_LEN);
		sec_int_set_cloud_hash_dec((void *)(buffer), &buf_len);

		LOG_INF("Decryption: cld hash val: %s\n", buffer);
		break;
	/* MBED decryption: token id End */

	default:
		LOG_INF("Wrong Decryption data type\n");
		break;
	}

	k_heap_free(&oob_sec_hc_heap, buffer);
	return ret;
}

static int sec_hc_response_data_once(
	sec_hc_cmd_data_type msg_type,
	uint32_t *buf_compose,
	uint32_t *a_sec_buffer,
	int len
	)
{
	int ret;
	union sec_hc_cmd_data_hdr_u res_hdr;

	res_hdr.bitfields.type = msg_type;
	res_hdr.bitfields.length = sizeof(union sec_hc_cmd_data_hdr_u)
				   + len;

	ret = sec_hc_create_cmd_data(
		buf_compose,
		HECI_COMPOSE_BUFFER_LEN,
		&sec_hc_resp_pkt.buf_compose_next,
		&sec_hc_resp_pkt.accum_compose_len,
		&res_hdr,
		(uint32_t *)a_sec_buffer);

	return ret;
}

static int sec_hc_response_data(sec_hc_type_resp_a sec_resp_data_type)
{
	int ret = SEC_SUCCESS;
	unsigned int data;
	int size;
	unsigned char buffer_pse_salt[SEC_PSE_SALT_LEN];

	memset(buffer_pse_salt, 0x00, SEC_PSE_SALT_LEN);
	sec_int_get_hkdf_32b_pse_salt(buffer_pse_salt, &size);

	switch (sec_resp_data_type) {
	case SEC_HC_TYPE_RESP_TOK:
		ret = sec_hc_response_data_once(
			SEC_HC_TYPE_ENC_TOK_ID,
			sec_hc_resp_pkt.buf_compose_next,
			(uint32_t *)sec_ctx.a_enc_tok_id,
			SEC_TOK_ID_LEN
			);
		if (ret != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Fail to Create Resp for Tok ID!\n",
				__func__, __LINE__);
			break;
		}
		ret = sec_hc_response_data_once(
			SEC_HC_TYPE_ENC_TOK_ID_IV,
			sec_hc_resp_pkt.buf_compose_next,
			(uint32_t *)sec_ctx.a_enc_tok_id_iv,
			SEC_ENC_IV_SIZE
			);
		if (ret != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Fail to Create Resp for Tok IV!\n",
				__func__, __LINE__);
			break;
		}
		sec_hc_response_data_once(
			SEC_HC_TYPE_ENC_TOK_ID_TAG,
			sec_hc_resp_pkt.buf_compose_next,
			(uint32_t *)sec_ctx.a_enc_tok_id_tag,
			SEC_ENC_TAG_SIZE
			);
		if (ret != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Fail to Create Resp for Tok TAG!\n",
				__func__, __LINE__);
		}
		break;

	case SEC_HC_TYPE_RESP_DEV:
		ret = sec_hc_response_data_once(
			SEC_HC_TYPE_ENC_DEV_ID,
			sec_hc_resp_pkt.buf_compose_next,
			(uint32_t *)sec_ctx.a_enc_dev_id,
			SEC_DEV_ID_LEN
			);
		if (ret != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Fail to Create Resp for Dev ID!\n",
				__func__, __LINE__);
			break;
		}
		ret = sec_hc_response_data_once(
			SEC_HC_TYPE_ENC_DEV_ID_IV,
			sec_hc_resp_pkt.buf_compose_next,
			(uint32_t *)sec_ctx.a_enc_dev_id_iv,
			SEC_ENC_IV_SIZE
			);
		if (ret != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Fail to Create Resp for Dev IV!\n",
				__func__, __LINE__);
			break;
		}
		ret = sec_hc_response_data_once(
			SEC_HC_TYPE_ENC_DEV_ID_TAG,
			sec_hc_resp_pkt.buf_compose_next,
			(uint32_t *)sec_ctx.a_enc_dev_id_tag,
			SEC_ENC_TAG_SIZE
			);
		if (ret != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Fail to Create Resp for Dev TAG!\n",
				__func__, __LINE__);
		}
		break;

	case SEC_HC_TYPE_RESP_MQTT_ID:
		LOG_INF("Create Resp data for mqtt ID\n");
		ret = sec_hc_response_data_once(
			SEC_HC_TYPE_ENC_MQTT_CLIENT_ID,
			sec_hc_resp_pkt.buf_compose_next,
			(uint32_t *)sec_ctx.a_enc_mqtt_client_id,
			SEC_MQTT_CLIENT_ID_LEN
			);
		if (ret != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Fail to Create Resp for Mqtt ID!\n",
				__func__, __LINE__);
			break;
		}
		ret = sec_hc_response_data_once(
			SEC_HC_TYPE_ENC_MQTT_CLIENT_ID_IV,
			sec_hc_resp_pkt.buf_compose_next,
			(uint32_t *)sec_ctx.a_enc_mqtt_client_id_iv,
			SEC_ENC_IV_SIZE
			);
		if (ret != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Fail to Create Resp for Mqtt IV!\n",
				__func__, __LINE__);
			break;
		}
		ret = sec_hc_response_data_once(
			SEC_HC_TYPE_ENC_MQTT_CLIENT_ID_TAG,
			sec_hc_resp_pkt.buf_compose_next,
			(uint32_t *)sec_ctx.a_enc_mqtt_client_id_tag,
			SEC_ENC_TAG_SIZE
			);
		if (ret != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Fail to Create Resp for Mqtt TAG!\n",
				__func__, __LINE__);
		}
		break;

	case SEC_HC_TYPE_RESP_PSE_SALT:
		ret = sec_hc_response_data_once(
			SEC_HC_TYPE_HKDF_32B_PSE_SALT,
			sec_hc_resp_pkt.buf_compose_next,
			(uint32_t *)buffer_pse_salt,
			SEC_PSE_SALT_LEN
			);
		if (ret != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Fail to Create Resp for PSE Salt!\n",
				__func__, __LINE__);
		}
		break;

	case SEC_HC_TYPE_RESP_CLD_HASH:
		ret = sec_hc_response_data_once(
			SEC_HC_TYPE_ENC_CLOUD_HASH_ID,
			sec_hc_resp_pkt.buf_compose_next,
			(uint32_t *)sec_ctx.a_enc_cloud_hash_id,
			SEC_CLOUD_HASH_LEN
			);
		if (ret != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Fail to Create Resp for Cld Hash!\n",
				__func__, __LINE__);
			break;
		}
		ret = sec_hc_response_data_once(
			SEC_HC_TYPE_ENC_CLOUD_HASH_ID_IV,
			sec_hc_resp_pkt.buf_compose_next,
			(uint32_t *)sec_ctx.a_enc_cloud_hash_id_iv,
			SEC_ENC_IV_SIZE
			);
		if (ret != SEC_SUCCESS) {
			LOG_DBG("[%s:%d]Fail to Create Resp for Cld Hash IV!\n",
				__func__, __LINE__);
			break;
		}
		ret = sec_hc_response_data_once(
			SEC_HC_TYPE_ENC_CLOUD_HASH_ID_TAG,
			sec_hc_resp_pkt.buf_compose_next,
			(uint32_t *)sec_ctx.a_enc_cloud_hash_id_tag,
			SEC_ENC_TAG_SIZE
			);
		if (ret != SEC_SUCCESS) {
			LOG_DBG("[%s:%d]Fail to Create Resp for Cld Hash TAG\n",
				__func__, __LINE__);
		}
		break;

	case SEC_HC_TYPE_RESP_OOB_ST:
		size = sizeof(unsigned int);
		sec_int_get_prov_state(&data);
		LOG_DBG("Check state of PSE OOB: %d\n", data);
		ret = sec_hc_response_data_once(
			SEC_HC_TYPE_OOB_STATUS,
			sec_hc_resp_pkt.buf_compose,
			&data,
			size
			);
		if (ret != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Fail to Create Resp for OOB Status!\n",
				__func__, __LINE__);
		}
		break;

	default:
		LOG_INF("Wrong Response data type\n");
		break;
	}

	return ret;
}

static int sec_hc_response_msg_type(
	sec_hc_cmd_data_type msg_type
	)
{
	int ret;
	int size;
	unsigned int data;

	LOG_INF("%s: Create response for msg type: %d\n", __func__, msg_type);
	size = sizeof(unsigned int);
	sec_int_get_prov_state(&data);
	LOG_DBG("OOB2 Flow, OOB State: %d\n", data);
	ret = sec_hc_response_data_once(
		msg_type,
		sec_hc_resp_pkt.buf_compose,
		&data,
		size
		);
	if (ret != SEC_SUCCESS) {
		LOG_DBG("[%s:%d] Failed to Create Resp for OOB2 Msg!\n",
			__func__, __LINE__);
	}
	return ret;
}

static int sec_hc_set_pref_cse_seed(a_cse_seed_type seed)
{
	int ret;

	LOG_INF("%s : Pref Seed idx: %d\n", __func__, seed);
	sec_hc_cse_seed.a_cse_seed_len = SEC_CSE_SEED_LEN;
	ret = sec_int_set_pse_32b_fuse(sec_hc_cse_seed.a_cse_seed[seed],
				       &sec_hc_cse_seed.a_cse_seed_len);

	if (ret != SEC_SUCCESS) {
		LOG_DBG("[%s:%d] PSE FUSE Set Failed!\n", __func__, __LINE__);
		return ret;
	}
	sec_hc_print_context_param("Set pref cse seed: ",
				   (uint8_t *)sec_hc_cse_seed.a_cse_seed[seed],
				   SEC_CSE_SEED_LEN);
	return ret;
}

/*
 * sec_hc_find_cse_seed will be invoked in oob2 case
 * to get right cse seed to decrypt received encrypted
 * data
 */
static unsigned int sec_hc_find_cse_seed(void)
{
	LOG_INF("%s: Totoal Seeds Available: %d\n",
		__func__, sec_hc_cse_seed.a_cse_seed_no);

	for (int idx = 0; idx < sec_hc_cse_seed.a_cse_seed_no; idx++) {

		if (sec_hc_set_pref_cse_seed(idx) != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Preferred CSE Seed Set Failed!\n",
				__func__, __LINE__);
			break;
		}

		LOG_INF("Cloud hash decrypt using preferred seed idx: %d\n",
			idx);
		if (sec_hc_decrypt_data(SEC_HC_TYPE_DEC_CLD_HASH,
					SEC_CLOUD_HASH_LEN) !=
				SEC_SUCCESS) {
			LOG_DBG("Retry with Previous seed value\n");
			continue;
		}
		LOG_INF("Pref Seed Value is at idx: %d\n", idx);
		if (idx != 0) {
			/* its expected idx=0 is preferred seed
			 * <CSE_SEED1>. Seed is no valid in case of idx
			 * is non zero, Reprovisioning is required,
			 * update reprovisioning state
			 */
			sec_hc_seed_updated = true;
			LOG_INF("Seed Value got updated\n");
			LOG_INF("Seed idx: %d, Set Reprovision Flag : %d\n",
				idx, sec_hc_seed_updated);
		}
		LOG_INF("Exit: %s\n", __func__);
		return SEC_SUCCESS;
	}
	LOG_INF("%s: Failed, Recheck Enc data OR CSE Seed Val\n", __func__);
	return SEC_FAILED;
}

void sec_hc_pse_set_salt(void)
{
	int ret;
	unsigned char buffer_pse_salt[SEC_PSE_SALT_LEN];
	int size = SEC_PSE_SALT_LEN;

	memset(buffer_pse_salt, 0x00, SEC_PSE_SALT_LEN);

	/* TRNG hardware generated seed in MBEDTLS */
	ret = sec_hc_gen_random_num(
		(unsigned char *)buffer_pse_salt,
		SEC_PSE_SALT_LEN
		);
	if (ret != SEC_SUCCESS) {
		LOG_INF("hardware generated seed Failed\n");
	}
	sec_hc_print_context_param(
		"Set PSE Salt, buffer_pse_salt: ",
		(uint8_t *)buffer_pse_salt,
		SEC_PSE_SALT_LEN);
	sec_int_set_hkdf_32b_pse_salt((void *)(buffer_pse_salt), &size);
}

static void sec_hc_process_cmd(
	uint8_t *buf
	)
{
	LOG_INF("Enter: %s\n", __func__);
	SEC_HC_PACKET_DATA_T *sec_hc_pkt;

	/* setup rxhdr*/
	sec_hc_pkt = sec_hc_setup_rxhdr(buf);
	if (sec_hc_pkt == NULL) {
		LOG_INF("Setup for rx header Buffer initialization Fail!\n");
		goto err;
	}

	/* setup txhdr */
	sec_hc_txhdr_config(sec_hc_pkt);

	/* initialize response packate */
	if (sec_hc_resp_data_int(sec_hc_pkt) != SEC_SUCCESS) {
		LOG_INF("Response Buffer initialization Fail!\n");
		k_free(sec_hc_pkt);
		goto err;
	}

	mbedtls_gcm_init(&sec_hc_cipher_t.gcm);

	mrd_t sec_hc_msg = { 0 };

	sec_hc_msg.buf = sec_hc_tx_buffer;

	LOG_INF("[sec_heci] %s\n", __func__);

	switch (sec_hc_pkt->rxhdr->command) {
	case SEC_HC_PROV_INIT_OOB:
		LOG_INF(">> [SEC_HC_PROV_INIT_OOB]\n");

		sec_hc_process_cmd_data(
			sec_hc_pkt->req_data,
			((sec_hc_pkt->rxhdr->length)
			 - sizeof(struct sec_hc_cmd_hdr_t)),
			&(sec_ctx.sec_ctx), !(sec_hc_pkt->rxhdr->msg_comp));

		sec_hc_pkt->txhdr->command = SEC_HC_PROV_INIT_OK;
		if ((sec_hc_pkt->rxhdr->msg_comp) != SEC_HC_LAST_MSG_YES) {
			LOG_INF("Multiple IPC packet >>\n");
			sec_hc_prev_command_id = SEC_HC_PROV_INIT_OOB;
			break;
		}

		/* Generate PSE Salt */
		sec_hc_pse_set_salt();
		LOG_INF("SEC_HC_PROV_INIT_OOB, Set pref cse seed as seed\n");
		if (sec_hc_set_pref_cse_seed(CSE_SEED1) != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Preferred CSE Seed Set Failed!\n",
				__func__, __LINE__);
			break;
		}

		/* Data type is token */
		LOG_INF("Start: Encrypt Token id\n");
		if (sec_hc_encrypt_data(SEC_HC_TYPE_ENC_TOK,
					SEC_TOK_ID_LEN) !=
				SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Start: Enc Tok id Fail!\n",
				__func__, __LINE__);
			break;
		}

		/* Data type is device */
		LOG_INF("Star: Encrypt Device id\n");
		if (sec_hc_encrypt_data(SEC_HC_TYPE_ENC_DEV,
					SEC_DEV_ID_LEN) !=
				SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Start: Enc Dev id Fail!\n",
				__func__, __LINE__);
			break;
		}

		/* Data type mqtt ID */
		LOG_INF("Start: Encrypt MQTT Client id\n");
		if (sec_hc_encrypt_data(SEC_HC_TYPE_ENC_MQTT_ID,
					SEC_MQTT_CLIENT_ID_LEN) !=
				SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Start: Enc MQTT Client id Fail!\n",
				__func__, __LINE__);
			break;
		}

		/* Data type cld hash ID */
		LOG_INF("Start: Encrypt CLD HASH id\n");
		if (sec_hc_encrypt_data(SEC_HC_TYPE_ENC_CLD_HASH,
					SEC_CLOUD_HASH_LEN) !=
				SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Start: Encrypt CLD HASH id Fail!\n",
				__func__, __LINE__);
			break;
		}

		/* Create response packet */
		sec_hc_resp_pkt.buf_compose =
			(uint32_t  *)(sec_hc_pkt->txhdr + 1);

		sec_hc_resp_pkt.buf_compose_next =
			(uint32_t  *)(sec_hc_pkt->txhdr + 1);

		if (sec_hc_response_data(SEC_HC_TYPE_RESP_PSE_SALT) !=
				SEC_SUCCESS) {
			LOG_DBG("Create Resp for PSE SALT Fail!\n");
			break;
		}

		if (sec_hc_response_data(SEC_HC_TYPE_RESP_TOK) !=
				SEC_SUCCESS) {
			LOG_DBG("Create Resp for Token ID Fail!\n");
			break;
		}

		if (sec_hc_response_data(SEC_HC_TYPE_RESP_DEV) !=
				SEC_SUCCESS) {
			LOG_DBG("Create Resp for Device ID!\n");
			break;
		}

		if (sec_hc_response_data(SEC_HC_TYPE_RESP_MQTT_ID) !=
				SEC_SUCCESS) {
			LOG_DBG("Create Resp for MQTT ID!\n");
			break;
		}

		if (sec_hc_response_data(SEC_HC_TYPE_RESP_CLD_HASH) !=
				SEC_SUCCESS) {
			LOG_DBG("Create Resp for CLD hash Fail!\n");
			break;
		}

		sec_hc_pkt->txhdr->length
			= sec_hc_resp_pkt.accum_compose_len;
		sec_hc_msg.len = sizeof(struct sec_hc_cmd_hdr_t)
				 + sec_hc_resp_pkt.accum_compose_len;

		/* RESPONSE */
		LOG_INF("Send Response for: SEC_HC_PROV_INIT_OOB\n");
		heci_send(sec_hc_conn_id, &sec_hc_msg);

		LOG_INF("[%s][[\n", __func__);
		sec_hc_print_context_param(
			"[get tx data]",
			(uint8_t *)(sec_hc_tx_buffer),
			(sec_hc_resp_pkt.accum_compose_len));
		LOG_INF("]]\n");
		LOG_INF("[SEC_HC_PROV_INIT_OK] >>\n");

		sec_hc_prev_command_id = SEC_HC_PROV_INIT_OOB;
		break;

	case SEC_HC_PROV_INIT_ACK:
		LOG_INF(">> [SEC_HC_PROV_INIT_ACK]\n");

		sec_hc_process_cmd_data(
			sec_hc_pkt->req_data,
			((sec_hc_pkt->rxhdr->length)
			 - sizeof(struct sec_hc_cmd_hdr_t)),
			&(sec_ctx.sec_ctx),
			!(sec_hc_pkt->rxhdr->msg_comp));

		sec_hc_pkt->txhdr->command = SEC_HC_PROV_INIT_ACK2;
		if ((sec_hc_pkt->rxhdr->msg_comp) != SEC_HC_LAST_MSG_YES) {
			LOG_INF("Multiple IPC packet >>\n");
			sec_hc_prev_command_id = SEC_HC_PROV_INIT_ACK;
			break;
		}
		sec_hc_pkt->txhdr->length
			= sec_hc_resp_pkt.accum_compose_len;
		sec_hc_msg.len = sizeof(struct sec_hc_cmd_hdr_t)
				 + sec_hc_resp_pkt.accum_compose_len;

		/* RESPONSE */
		LOG_INF("Send Response for: SEC_HC_PROV_INIT_ACK\n");
		heci_send(sec_hc_conn_id, &sec_hc_msg);

		LOG_INF("[%s][[\n", __func__);
		sec_hc_print_context_param(
			"[get tx data]",
			(uint8_t *)(sec_hc_tx_buffer),
			(sec_hc_resp_pkt.accum_compose_len));
		LOG_INF("]]\n");

		sec_hc_prev_command_id = SEC_HC_PROV_INIT_ACK;
		LOG_INF("[SEC_HC_PROV_INIT_ACK2] >>\n");
		break;

	case SEC_HC_STATUS_OOB:
		LOG_INF(">> [SEC_HC_STATUS_OOB]\n");

		sec_hc_pkt->txhdr->command = SEC_HC_STATUS_OOB_RES;
		if (sec_hc_response_data(SEC_HC_TYPE_RESP_OOB_ST) !=
				SEC_SUCCESS) {
			LOG_DBG("Create Resp OOB Status Fail!\n");
			break;
		}
		sec_hc_pkt->txhdr->length =
			sec_hc_resp_pkt.accum_compose_len;
		sec_hc_msg.len = sizeof(
			struct sec_hc_cmd_hdr_t)
				 + sec_hc_resp_pkt.accum_compose_len;

		/* RESPONSE */
		LOG_INF("Send Response for: SEC_HC_STATUS_OOB\n");
		heci_send(sec_hc_conn_id, &sec_hc_msg);

		LOG_INF("[%s][[\n", __func__);
		sec_hc_print_context_param(
			"[get tx data]",
			(uint8_t *)(sec_hc_tx_buffer),
			(sec_hc_resp_pkt.accum_compose_len));
		LOG_INF("]]\n");

		sec_hc_prev_command_id = SEC_HC_STATUS_OOB;
		LOG_INF("[SEC_HC_STATUS_OOB_RES] >>\n");
		break;

	case SEC_HC_INIT_OOB:
		LOG_INF(">> [SEC_HC_INIT_OOB]\n");

		sec_hc_process_cmd_data(
			sec_hc_pkt->req_data,
			((sec_hc_pkt->rxhdr->length) -
			 sizeof(struct sec_hc_cmd_hdr_t)),
			&(sec_ctx.sec_ctx),
			!(sec_hc_pkt->rxhdr->msg_comp));

		sec_hc_pkt->txhdr->command = SEC_HC_INIT_OOB_OK;
		if ((sec_hc_pkt->rxhdr->msg_comp) !=
		    SEC_HC_LAST_MSG_YES) {
			LOG_INF("Multiple IPC packet >>\n");
			sec_hc_prev_command_id = SEC_HC_INIT_OOB;
			break;
		}

		sec_hc_pkt->txhdr->length =
			sec_hc_resp_pkt.accum_compose_len;
		sec_hc_msg.len = sizeof(struct sec_hc_cmd_hdr_t) +
				 sec_hc_resp_pkt.accum_compose_len;

		/* RESPONSE */
		LOG_INF("Send Response for: SEC_HC_INIT_OOB\n");
		heci_send(sec_hc_conn_id, &sec_hc_msg);

		LOG_INF("[%s][[\n", __func__);
		sec_hc_print_context_param(
			"[get tx data]",
			(uint8_t *)(sec_hc_tx_buffer),
			(sec_hc_resp_pkt.accum_compose_len));
		LOG_INF("]]\n");
		LOG_INF("[SEC_HC_INIT_OOB_OK] >>\n");

		sec_hc_prev_command_id = SEC_HC_INIT_OOB;
		break;

	case SEC_HC_INIT_OOB2:
		LOG_INF(" >> [SEC_HC_INIT_OOB2]\n");
		sec_hc_process_cmd_data(
			sec_hc_pkt->req_data,
			((sec_hc_pkt->rxhdr->length)
			 - sizeof(struct sec_hc_cmd_hdr_t)),
			&(sec_ctx.sec_ctx),
			!(sec_hc_pkt->rxhdr->msg_comp));

		sec_hc_pkt->txhdr->command = SEC_HC_INIT_ACK;
		if ((sec_hc_pkt->rxhdr->msg_comp) != SEC_HC_LAST_MSG_YES) {
			LOG_INF("Multiple IPC packet >>\n");
			sec_hc_prev_command_id = SEC_HC_INIT_OOB2;
			break;
		}

		/* First decrypt hash value and
		 * check decrypted value with received
		 * hash value from BIOS
		 * [normal flow bios will send
		 * enc hash data and hash value ]
		 */

		LOG_INF("call -> sec_hc_find_cse_seed\n");
		if (sec_hc_find_cse_seed() != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Failed to Find Right Seed Val!\n",
				__func__, __LINE__);
			if (sec_hc_response_msg_type(
				    SEC_HC_TYPE_NORMAL_FLW_DATA_ERR) !=
					SEC_SUCCESS) {
				LOG_DBG("Create Resp for data err Failed!\n");
				break;
			}
			goto resp;
		}

		/* Compare Two HASH Val Dec val with Received value */
		if (sec_hc_int_hash_check() != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] CLD Hash check Failed !\n",
				__func__, __LINE__);
			if (sec_hc_response_msg_type(
				    SEC_HC_TYPE_NORMAL_FLW_HASH_ERR)
					!= SEC_SUCCESS) {
				LOG_DBG(
					"Create Resp for Hash Err Failed!\n");
				break;
			}
			goto resp;
		}

		if (sec_hc_seed_updated == true) {
			LOG_INF("Reprovision set in sec ctx\n");
			unsigned int reprov_state =
				(unsigned int)SEC_RPROV;
			sec_int_set_prov_state(
				(unsigned int *)&reprov_state);
			if (sec_hc_response_msg_type(
					SEC_HC_TYPE_NORMAL_FLW_REPROV_REQ)
					!= SEC_SUCCESS) {
				LOG_DBG("Create Resp for Reprov Failed!\n");
				break;
			}
			goto decry;
		}
		if (sec_hc_response_msg_type(SEC_HC_TYPE_NORMAL_FLW_SUCCESS) !=
				SEC_SUCCESS) {
			LOG_DBG("Create Resp for Normal Flow Failed!\n");
			break;
		}

decry:
		/* Decrypt token ID */
		LOG_INF("Start: Decrypt Token ID\n");
		if (sec_hc_decrypt_data(SEC_HC_TYPE_DEC_TOK,
					SEC_TOK_ID_LEN) !=
				SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Start: Decrypt Token id Fail!\n",
				__func__, __LINE__);
			break;
		}

		/* Decrypt Device ID */
		LOG_INF("Start: Decrypt Device ID\n");
		if (sec_hc_decrypt_data(SEC_HC_TYPE_DEC_DEV,
					SEC_DEV_ID_LEN) !=
				SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Start: Decrypt Device id Fail!\n",
				__func__, __LINE__);
			break;
		}

		/* Decrypt mqtt ID */
		LOG_INF("Start: Decrypt MQTT Client ID\n");
		if (sec_hc_decrypt_data(SEC_HC_TYPE_DEC_MQTT_ID,
					SEC_MQTT_CLIENT_ID_LEN) !=
				SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Start: Encrypt MQTT id Fail!\n",
				__func__, __LINE__);
			break;
		}

		/*
		 * Create response packet
		 */
resp:
		sec_hc_pkt->txhdr->length =
			sec_hc_resp_pkt.accum_compose_len;
		sec_hc_msg.len = sizeof(struct sec_hc_cmd_hdr_t)
				 + sec_hc_resp_pkt.accum_compose_len;

		/* RESPONSE */
		LOG_INF("Send Response for: SEC_HC_INIT_OOB2\n");
		heci_send(sec_hc_conn_id, &sec_hc_msg);

		LOG_INF("[%s][[\n", __func__);
		sec_hc_print_context_param(
			"[get tx data] ",
			(uint8_t *)(sec_hc_tx_buffer),
			(sec_hc_resp_pkt.accum_compose_len));
		LOG_INF("]]\n");
		LOG_INF("[SEC_HC_INIT_ACK] >>\n");

		sec_hc_prev_command_id = SEC_HC_INIT_OOB2;
		break;

	case SEC_HC_INIT_ACK2:
		LOG_INF(" >> [SEC_HC_INIT_ACK2]\n");

		sec_hc_process_cmd_data(
			sec_hc_pkt->req_data,
			((sec_hc_pkt->rxhdr->length)
			 - sizeof(struct sec_hc_cmd_hdr_t)),
			&(sec_ctx.sec_ctx),
			!(sec_hc_pkt->rxhdr->msg_comp));

		sec_hc_prev_command_id = SEC_HC_INIT_ACK2;
		sec_hc_rel_cse_seed();

		/* Flag that the init flow completed */
		sec_int_lock_ctx_mutex();
		sec_int_start_init_flag = 1;
		sec_int_unlock_ctx_mutex();
		break;

	case SEC_HC_RPROV_REQ:
		LOG_INF(" >> [SEC_HC_RPROV_REQ]\n");

		sec_hc_seed_updated = false;
		sec_hc_pkt->txhdr->command = SEC_HC_RPROV_RES;

		LOG_INF("Create pse seed salt using pse hw random gen\n");
		sec_hc_pse_set_salt();

		if (sec_hc_set_pref_cse_seed(CSE_SEED1) != SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Preferred CSE Seed Set Failed!\n",
				__func__, __LINE__);
			break;
		}

		/* Data type is token */
		LOG_INF("Start: Encrypt Token id\n");
		if (sec_hc_encrypt_data(SEC_HC_TYPE_ENC_TOK,
					SEC_TOK_ID_LEN) !=
				SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Start: Encrypt Token id Failed!\n",
				__func__, __LINE__);
			break;
		}

		/* Data type is device */
		LOG_INF("Start: Encrypt Device id\n");
		if (sec_hc_encrypt_data(SEC_HC_TYPE_ENC_DEV,
					SEC_DEV_ID_LEN) !=
				SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Start: Encrypt Device id Failed!\n",
				__func__, __LINE__);
			break;
		}

		/* encrypt mqtt ID */
		LOG_INF("Start: Encrypt MQTT Client id\n");
		if (sec_hc_encrypt_data(SEC_HC_TYPE_ENC_MQTT_ID,
					SEC_MQTT_CLIENT_ID_LEN) !=
				SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Start: Encrypt MQTT id Failed!\n",
				__func__, __LINE__);
			break;
		}

		/* Data type cld hash ID */
		LOG_INF("Start: Encrypt CLD HASH id\n");
		if (sec_hc_encrypt_data(SEC_HC_TYPE_ENC_CLD_HASH,
					SEC_CLOUD_HASH_LEN) !=
				SEC_SUCCESS) {
			LOG_DBG("[%s:%d] Start: Encrypt CLD HASH id Failed!\n",
				__func__, __LINE__);
			break;
		}

		sec_hc_resp_pkt.buf_compose =
			(uint32_t  *)(sec_hc_pkt->txhdr + 1);
		sec_hc_resp_pkt.buf_compose_next =
			(uint32_t  *)(sec_hc_pkt->txhdr + 1);

		if (sec_hc_response_data(SEC_HC_TYPE_RESP_PSE_SALT) !=
				SEC_SUCCESS) {
			LOG_DBG("Create Resp for PSE SALT Failed!\n");
			break;
		}

		if (sec_hc_response_data(SEC_HC_TYPE_RESP_TOK) !=
				SEC_SUCCESS) {
			LOG_DBG("Create Resp for Token ID Failed!\n");
			break;
		}

		if (sec_hc_response_data(SEC_HC_TYPE_RESP_DEV) !=
				SEC_SUCCESS) {
			LOG_DBG("Create Resp for Device ID!\n");
			break;
		}

		if (sec_hc_response_data(SEC_HC_TYPE_RESP_MQTT_ID) !=
				SEC_SUCCESS) {
			LOG_DBG("Create Resp for MQTT ID!\n");
			break;
		}

		if (sec_hc_response_data(SEC_HC_TYPE_RESP_CLD_HASH) !=
				SEC_SUCCESS) {
			LOG_DBG("Create Resp for CLD hash Failed!\n");
			break;
		}

		sec_hc_pkt->txhdr->length =
			sec_hc_resp_pkt.accum_compose_len;
		sec_hc_msg.len = sizeof(struct sec_hc_cmd_hdr_t)
				 + sec_hc_resp_pkt.accum_compose_len;

		/* RESPONSE */
		LOG_INF("Send Response for: SEC_HC_RPROV_REQ\n");
		heci_send(sec_hc_conn_id, &sec_hc_msg);

		LOG_INF("[%s][[\n", __func__);
		sec_hc_print_context_param(
			"[get tx data] ",
			(uint8_t *)(sec_hc_tx_buffer),
			(sec_hc_resp_pkt.accum_compose_len));
		LOG_INF("]]\n");
		LOG_INF("[SEC_HC_RPROV_RES] >>\n ");

		sec_hc_prev_command_id = SEC_HC_RPROV_REQ;
		break;

	case SEC_HC_RPROV_ACK:
		LOG_INF(" >> [SEC_HC_RPROV_ACK]\n");

		unsigned int prov_state =
			(unsigned int)SEC_PROV;
		sec_int_set_prov_state(
			(unsigned int *)&prov_state);
		sec_hc_pkt->txhdr->command = SEC_HC_RPROV_ACK2;
		sec_hc_pkt->txhdr->length =
			sec_hc_resp_pkt.accum_compose_len;
		sec_hc_msg.len = sizeof(struct sec_hc_cmd_hdr_t)
				 + sec_hc_resp_pkt.accum_compose_len;

		LOG_INF("Send Response for: SEC_HC_RPROV_ACK\n");
		heci_send(sec_hc_conn_id, &sec_hc_msg);

		LOG_INF("[%s][[\n", __func__);
		sec_hc_print_context_param(
			"[get tx data]",
			(uint8_t *)(sec_hc_tx_buffer),
			(sec_hc_resp_pkt.accum_compose_len));
		LOG_INF("]]\n");
		LOG_INF("[SEC_HC_RPROV_ACK] >>\n");

		sec_hc_prev_command_id = SEC_HC_RPROV_ACK;
		sec_hc_rel_cse_seed();

		/* Flag that the init flow completed */
		sec_int_lock_ctx_mutex();
		sec_int_start_init_flag = 1;
		sec_int_unlock_ctx_mutex();
		break;

	case SEC_HC_DECOMM_ACK:
		LOG_INF(">> [SEC_HC_DECOMM_ACK]\n");

		/*
		 * BIOS Sent ACK as to perform remaining Decommission from
		 * PSE OOB side
		 */
		sec_hc_pkt->txhdr->command = SEC_HC_DECOMM_ACK2;
		sec_hc_pkt->txhdr->length
			= sec_hc_resp_pkt.accum_compose_len;
		sec_hc_msg.len = sizeof(struct sec_hc_cmd_hdr_t)
				 + sec_hc_resp_pkt.accum_compose_len;

		/* RESPONSE */
		LOG_INF("Send Response for: SEC_HC_DECOMM_ACK\n");
		heci_send(sec_hc_conn_id, &sec_hc_msg);

		LOG_INF("[%s][[\n", __func__);
		sec_hc_print_context_param(
			"[get tx data]",
			(uint8_t *)(sec_hc_tx_buffer),
			(sec_hc_resp_pkt.accum_compose_len));
		LOG_INF("]]\n");
		LOG_INF("[SEC_HC_DECOMM_ACK] >>\n");

		sec_hc_prev_command_id = SEC_HC_DECOMM_ACK;
		/* Notify OOB service to continue user side decommissioning */
		sec_hc_final_decommission();
		break;

	default:
		LOG_INF("get SEC_HC ANY command\n");
		LOG_INF("heci_send for ANY\n");
		break;
	}

	k_free(sec_hc_pkt);
	mbedtls_gcm_free(&sec_hc_cipher_t.gcm);

err:
	LOG_INF("Exiting %s\n", __func__);
}

/* ================================================================= */
static void sec_hc_event_callback(unsigned int event, void *param)
{
	ARG_UNUSED(param);

	LOG_INF("[sec_heci] %s: , Event: %u\n", __func__, event);
	sec_hc_event = event;
	k_sem_give(&sec_hc_event_sem);
}

static void sec_hc_task(
	void *p1,
	void *p2,
	void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int status;

	while (true) {
		status = k_sem_take(&sec_hc_event_sem, K_FOREVER);

		LOG_INF("[sec_heci] k_sem_take status: %d, new heci event %u\n",
			status, sec_hc_event);

		switch (sec_hc_event) {
		case HECI_EVENT_NEW_MSG:
			if (sec_hc_rx_msg.msg_lock != MSG_LOCKED) {
				LOG_INF("[sec_heci] invalid heci message\n");
				break;
			}

			if (sec_hc_rx_msg.type == HECI_CONNECT) {
				sec_hc_conn_id = sec_hc_rx_msg.connection_id;
				LOG_INF("[sec_heci] new conn: %u\n",
					sec_hc_conn_id);
			} else if (sec_hc_rx_msg.type == HECI_REQUEST) {
				sec_hc_process_cmd(sec_hc_rx_msg.buffer);
			}

			/*
			 * send flow control after finishing one message,
			 * allow host to send new request
			 */
			/* RESPONSE */
			heci_send_flow_control(sec_hc_conn_id);
			break;

		case HECI_EVENT_DISCONN:
			LOG_INF("[sec_heci]disconnect request conn %d\n",
				sec_hc_conn_id);
			heci_complete_disconnect(sec_hc_conn_id);
			if (sec_hc_abort_set) {
				LOG_INF("Exit Sec heci thread\n");
				k_thread_abort(sec_hc_thread_id);
			}
			break;

		default:
			LOG_INF("[sec_heci]wrong heci event %u\n",
				sec_hc_event);
			break;
		}
	}
}

static int sec_hc_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	int ret;

	sec_int_start_init_flag = 0;
	sec_int_init_context();

	/* PSE_FUSE is generated Now from CSE,
	 * CSE derived seed from BIOS IPC
	 * copied to fuse.
	 */
	memset(sec_ctx.a_pse_32b_fuse, 0x00, SEC_CSE_SEED_LEN);
	heci_client_t sec_hc_client = {
		.protocol_id = SEC_HC_GUID,
		.max_msg_size = SEC_HC_MAX_RX_SIZE,
		.protocol_ver = 1,
		.max_n_of_connections = 1,
		.dma_header_length = 0,
		.dma_enabled = 0,
		.rx_buffer_len = SEC_HC_MAX_RX_SIZE,
		.event_cb = sec_hc_event_callback,
	};

	sec_hc_client.rx_msg = &sec_hc_rx_msg;
	sec_hc_client.rx_msg->buffer = sec_hc_rx_buffer;

	LOG_INF("[sec_heci] enter %s\n", __func__);

	ret = heci_register(&sec_hc_client);
	if (ret) {
		LOG_INF("[sec_heci]failed to register heci client %d\n", ret);
		return ret;
	}

	sec_hc_thread_id = k_thread_create(&sec_hc_thread,
					   sec_hc_stack,
					   SEC_HC_STACK_SIZE,
					   sec_hc_task, NULL, NULL, NULL,
					   10, 0, K_NO_WAIT);

	return 0;
}

/* ================================================================= */

SYS_INIT(sec_hc_init, POST_KERNEL, 80);

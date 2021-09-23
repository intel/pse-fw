/**
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifndef PSE_OOB_SEC_HECI_CLIENT_H
#define PSE_OOB_SEC_HECI_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * In the data structures being exchange between OOB_SEC and BIOS,
 * indicating the data structure is coming from OOB_SEC or BIOS for
 * double confirmation.
 */
enum {
	/* SEC_HC Commands */
	SEC_HC_SOURCE_OOB       = 0x0,
	SEC_HC_SOURCE_BIOS      = 0x1
} sec_hc_cmd_source_flag;

/*
 * In the IPC handshake, indicating if the current IPC command type.
 */
typedef enum {
	/* SEC_HC Commands */
	SEC_HC_PROV_INIT_OOB    = 0x00,
	SEC_HC_PROV_INIT_OK     = 0x01,
	SEC_HC_PROV_INIT_ACK    = 0x02,
	SEC_HC_PROV_INIT_ACK2   = 0x03,

	SEC_HC_INIT_OOB         = 0x10,
	SEC_HC_INIT_OOB_OK      = 0x11,
	SEC_HC_INIT_OOB2        = 0x12,
	SEC_HC_INIT_ACK         = 0x13,
	SEC_HC_INIT_ACK2        = 0x14,

	/* Hash check failed */
	SEC_HC_INIT_NACK        = 0x15,

	/* Reprov flow */
	SEC_HC_RPROV_REQ        = 0x20,
	SEC_HC_RPROV_RES        = 0x21,
	SEC_HC_RPROV_ACK        = 0x22,
	SEC_HC_RPROV_ACK2       = 0x23,

	/*
	 * init flow
	 * Status command from BIOS OOB will respond
	 * with OOB status: Prov, Decomm, Prov_pend
	 */
	SEC_HC_STATUS_OOB       = 0x24,
	SEC_HC_STATUS_OOB_RES   = 0x25,

	/*
	 * BIOS  >> SEC_HC_DECOMM_ACK
	 * OOB >> SEC_HC_DECOMM_ACK2
	 */
	SEC_HC_DECOMM_OOB       = 0x26,
	SEC_HC_DECOMM_ACK       = 0x27,
	SEC_HC_DECOMM_ACK2      = 0x28,

	SEC_HC_TESTING          = 0xFF
} sec_hc_command_id;

/*
 * In the IPC handshake, indicating if the previous IPC received is
 * handled or decoded correctly to the sender.
 */
enum {
	/* SEC_HC Commands */
	SEC_HC_STATUS_OK        = 0x0,
	SEC_HC_STATUS_ERROR     = 0x1
} sec_hc_cmd_status_flag;

typedef enum {
	SEC_HC_TYPE_ENC_TOK,
	SEC_HC_TYPE_ENC_DEV,
	SEC_HC_TYPE_ENC_MQTT_ID,
	SEC_HC_TYPE_ENC_CLD_HASH
} sec_hc_type_enc_a;

typedef enum {
	SEC_HC_TYPE_DEC_TOK,
	SEC_HC_TYPE_DEC_DEV,
	SEC_HC_TYPE_DEC_MQTT_ID,
	SEC_HC_TYPE_DEC_CLD_HASH
} sec_hc_type_dec_a;

typedef enum {
	SEC_HC_TYPE_RESP_TOK,
	SEC_HC_TYPE_RESP_DEV,
	SEC_HC_TYPE_RESP_PSE_SALT,
	SEC_HC_TYPE_RESP_MQTT_ID,
	SEC_HC_TYPE_RESP_OOB_ST,
	SEC_HC_TYPE_RESP_CLD_HASH,
	SEC_HC_TYPE_RESP_OOB2
} sec_hc_type_resp_a;

/*
 * In the data structures being exchange between OOB_SEC and BIOS, indicating
 * the data structure is spanning more than one IPC packets if NO or indicating
 * the current packet is the last packet or the only packet
 */
enum {
	/* SEC_HC Commands */
	SEC_HC_LAST_MSG_NO      = 0x0,
	SEC_HC_LAST_MSG_YES     = 0x1
} sec_hc_cmd_completion_flag;

/*
 * In the credential data structures being exchange between OOB_SEC and BIOS,
 * indicating the data structure type per the clear credentials info or the
 * encrypted credentials info actually stored in BIOS.
 */
typedef enum {
	/* SEC_HC Commands */
	SEC_HC_TYPE_STATUS                      = 0x00,
	SEC_HC_TYPE_ROOTCA                      = 0x01,
	SEC_HC_TYPE_256_OWN_PUB_KEY             = 0x02,
	SEC_HC_TYPE_HKDF_32B_PSE_SALT           = 0x04,
	SEC_HC_TYPE_DENC_TOK_ID                 = 0x05,
	SEC_HC_TYPE_DENC_DEV_ID                 = 0x06,
	SEC_HC_TYPE_ENC_TOK_ID                  = 0x07,
	SEC_HC_TYPE_ENC_TOK_ID_TAG              = 0x08,
	SEC_HC_TYPE_ENC_TOK_ID_IV               = 0x09,
	SEC_HC_TYPE_ENC_DEV_ID                  = 0x0A,
	SEC_HC_TYPE_ENC_DEV_ID_TAG              = 0x0B,
	SEC_HC_TYPE_ENC_DEV_ID_IV               = 0x0C,
	SEC_HC_TYPE_CLD_URL                     = 0x0D,
	SEC_HC_TYPE_CLD_PORT                    = 0x0E,
	SEC_HC_TYPE_PXY_URL                     = 0x0F,
	SEC_HC_TYPE_PXY_PORT                    = 0x10,
	SEC_HC_TYPE_PROV_STATE                  = 0x11,
	SEC_HC_TYPE_PSE_SEED_1                  = 0x12,
	SEC_HC_TYPE_PSE_SEED_2                  = 0x13,
	SEC_HC_TYPE_PSE_SEED_3                  = 0x14,
	SEC_HC_TYPE_PSE_SEED_4                  = 0x15,
	SEC_HC_TYPE_CLD_ADAPTER                 = 0x17,
	SEC_HC_TYPE_DENC_MQTT_CLIENT_ID         = 0x18,
	SEC_HC_TYPE_ENC_MQTT_CLIENT_ID          = 0x19,
	SEC_HC_TYPE_ENC_MQTT_CLIENT_ID_TAG      = 0x1A,
	SEC_HC_TYPE_ENC_MQTT_CLIENT_ID_IV       = 0x1B,
	SEC_HC_TYPE_OOB_STATUS                  = 0x1C,
	SEC_HC_TYPE_DENC_CLOUD_HASH_ID          = 0x1D,
	SEC_HC_TYPE_ENC_CLOUD_HASH_ID           = 0x1E,
	SEC_HC_TYPE_ENC_CLOUD_HASH_ID_TAG       = 0x1F,
	SEC_HC_TYPE_ENC_CLOUD_HASH_ID_IV        = 0x20,
	SEC_HC_TYPE_NORMAL_FLW_SUCCESS          = 0x21,
	SEC_HC_TYPE_NORMAL_FLW_HASH_ERR         = 0x22,
	SEC_HC_TYPE_NORMAL_FLW_REPROV_REQ       = 0x23,
	SEC_HC_TYPE_NORMAL_FLW_DATA_ERR         = 0x24,

} sec_hc_cmd_data_type;

/*
 * This is the wrapper layer 1 generic data structure header being used
 * between IPC command exchange between BIOS and OOB_SEC.
 */
struct sec_hc_cmd_hdr_t {
	uint16_t msg_comp : 1;
	uint16_t rsvd : 2;
	uint16_t status : 1;
	uint16_t protocol_ver : 4;
	uint16_t source : 4;
	uint16_t length : 12;
	uint16_t command : 8;
}  __attribute__ ((__packed__));

/*
 * This is the wrapper layer 2 generic data structure header being used between
 * credential data structures exchange between BIOS and OOB_SEC embedded within
 * the IPC command structure.
 */
struct sec_hc_cmd_data_hdr_t {
	unsigned int type : 8;
	unsigned int length : 12;
	unsigned int rsvd : 12;
}  __attribute__ ((__packed__));

/*
 * A union version of the sec_hc_cmd_data_hdr_t structure to ease printf
 * logging and debug
 */
union sec_hc_cmd_data_hdr_u {
	struct sec_hc_cmd_data_hdr_t bitfields;
	unsigned int whole;
};

/*
 * This is the data structure for all credential data structure header as
 * defined in sec_hc_cmd_data_hdr_t.
 */
typedef struct {
	uint8_t *data;
} sec_hc_cmd_data_t;

#if (defined(SEC_DEBUG) && SEC_DEBUG)
/*
 * This is the debug structure for all credential data structure header for
 * endianness.
 */
struct {
	uint16_t data1;
	uint16_t data2;
	uint16_t data3;
	uint16_t data4;
} __packed oob_get_data_resp;
#endif

#ifdef __cplusplus
}
#endif

#endif /* PSE_OOB_SEC_HECI_CLIENT_H */

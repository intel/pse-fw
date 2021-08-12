/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PMC_SERVICE_H
#define PMC_SERVICE_H
/**
 * @brief PSE Services APIs
 * @defgroup pmc_services_interface PSE Service APIs
 * @{
 */

#include <drivers/sideband.h>

#define MAX_PECI_MSG_SIZE (20)

#define PMC_CLIENT_ECLITE (0)
#define PMC_CLIENT_OOB (1)

#define FORMAT_SHORT (0)
#define FORMAT_LONG (1)
#define FORMAT_SB_RAW_WRITE (2)
#define FORMAT_SB_RAW_READ (3)
#define FORMAT_HWSB_PME_REQ (4)
#define FORMAT_WIRE_GLOBAL_RESET (5)

#define PMC_WAIT_ACK  (1)
#define PMC_NO_WAIT_ACK (0)

struct short_msg_t {
	uint8_t cmd_id;
	uint8_t payload;
	uint32_t drbl_val;
};

struct pmc_msg_t {
	uint32_t client_id : 8;
	uint32_t format : 3;
	uint32_t wait_for_ack : 1;
	uint32_t msg_size : 10;
	union _msg_payload {
		struct short_msg_t short_msg;
		struct sb_raw_message raw_msg;
		uint8_t msg[MAX_PECI_MSG_SIZE];
	} u;
};

/**
 * @brief send pmc sync message.
 *
 * This routine sends a pmc sync message.
 *
 * @param usr_msg Pointer to user message
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -EINVAL if length > 8.
 */
int pmc_sync_send_msg(struct pmc_msg_t *usr_msg);

/**
 * @}
 */

#endif


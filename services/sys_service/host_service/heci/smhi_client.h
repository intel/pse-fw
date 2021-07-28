/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SMHI_CLIENT_H
#define __SMHI_CLIENT_H

#include <zephyr.h>
#include <string.h>
#include "heci.h"

#define REBOOT_FLAG BIT(0)
#define SMHI_CONN_FLAG BIT(1)

#define PSE_VNN_STATUS_REG0 0x4050003c
#define PSE_VNN_STATUS_REG1 0x4050006c

/* SMHI Commands */
typedef enum {
	/*retrieve PSE system info*/
	SMHI_GET_VERSION        = 0x1,
	SMHI_GET_TIME           = 0x8,
	/* System control*/
	SMHI_PSE_RESET          = 0x2,
	SMHI_COMMAND_LAST
} smhi_command_id;

typedef struct {
	uint8_t command         : 7;
	uint8_t is_response     : 1;
	uint16_t has_next       : 1;
	uint16_t reserved       : 15;
	uint8_t status;
} __packed smhi_msg_hdr_t;

typedef struct {
	uint16_t major;
	uint16_t minor;
	uint16_t hotfix;
	uint16_t build;
} __packed smhi_get_version_resp;

typedef struct {
	uint64_t rtc_us;
	/*below reserved to adapt ISH*/
	uint64_t tsync_counter;
	uint64_t rtc_us_do_sync;
	uint64_t tsync_counter_do_sync;
	uint64_t mia_frequency;
	uint64_t tsync_freq;
} __packed smhi_get_time_resp;

typedef struct {
	uint32_t reserved;
} __packed smhi_reset_resp;
#endif /* __SMHI_CLIENT_H */

/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SMPL_CLIENT_H
#define __SMPL_CLIENT_H

#include "heci.h"
#include <string.h>
#include <zephyr.h>

#define REBOOT_FLAG BIT(0)
#define SMPL_CONN_FLAG BIT(1)

#define SMPL_MAJOR_VERSION 0
#define SMPL_MINOR_VERSION 1
#define SMPL_HOTFIX_VERSION 2
#define SMPL_BUILD_VERSION 3

/* SMPL Commands */
typedef enum {
	/* retrieve PSE system info */
	SMPL_GET_VERSION = 0x1,
	SMPL_COMMAND_LAST
} smpl_command_id;

typedef struct {
	uint8_t command : 7;
	uint8_t is_response : 1;
	uint16_t has_next : 1;
	uint16_t reserved : 15;
	uint8_t status;
} __packed smpl_msg_hdr_t;

typedef struct {
	uint16_t major;
	uint16_t minor;
	uint16_t hotfix;
	uint16_t build;
} __packed smpl_get_version_resp;

#endif /* __SMPL_CLIENT_H */

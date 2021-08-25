/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <common.h>
#include <pmc_service.h>
#include "string.h"
#include "thermal_framework.h"
#include "eclite_hw_interface.h"
#include "platform.h"

LOG_MODULE_REGISTER(eclite_pmc, CONFIG_ECLITE_LOG_LEVEL);

static APP_GLOBAL_VAR_BSS(1) struct pmc_msg_t peci_msg;
static APP_GLOBAL_VAR_BSS(1) int8_t tj_max;

#define SHORT_MSG_LEN   4
#define REQ_LEN         5
#define RESP_LEN        4
#define COMPLETION_CODE 0x40

#define SHORT_MSG_HEADER(wait) {			\
		memset(&peci_msg, 0, sizeof(peci_msg));	\
		peci_msg.client_id = PMC_CLIENT_ECLITE;	\
		peci_msg.format = FORMAT_SHORT;		\
		peci_msg.wait_for_ack = wait;		\
		peci_msg.msg_size = SHORT_MSG_LEN;	\
}

#define LONG_MSG_HEADER(wait, size1) {			\
		memset(&peci_msg, 0, sizeof(peci_msg));	\
		peci_msg.client_id = PMC_CLIENT_ECLITE;	\
		peci_msg.format = FORMAT_LONG;		\
		peci_msg.wait_for_ack = wait;		\
		peci_msg.msg_size = size1;		\
}

#define PMC_MSG_HEADER_SIZE     4
#define PECI_TARGET_ADDRESS     0x30
#define TJ_MAX                  tj_max

#define PECI_FAIL_SAFE_TJMAX    100
#define PECI_FAIL_SAFE_TEMP     72

/* Get Temp Macros*/
#define RAW_TO_INT              6

enum peci_rd_pkg_index {
	PECI_TJMAX_INDEX = 0x10,
};

enum peci_command_code {
	PECI_GET_TEMP_CMD = 1,
	PECI_RD_PKG_CMD = 0xA1,
};

enum pmc_cmd_id {
	PMC_PECI_GET_TEMP_CMD = 0x12,
	PMC_POWER_STATE_CHANGE_CMD = 0x14,
	PMC_PECI_RD_PKG_CMD = 0x15,
};

struct get_temp_req_t {
	uint16_t cmd;
	uint16_t rsvd;
	uint8_t peci_adr;
	uint8_t wr_len;
	uint8_t rd_len;
	uint8_t peci_cmd;
};

struct get_temp_res_t {
	uint16_t cmd;
	uint16_t rsvd;
	uint8_t peci_res;
	uint8_t res_lsb;
	uint8_t res_msb;
};

struct read_package_req_t {
	uint16_t cmd;
	uint16_t rsvd;
	uint8_t peci_add;
	uint8_t wr_len;
	uint8_t rd_len;
	uint8_t peci_cmd;
	uint8_t host_id_retry;
	uint8_t index;
	uint8_t param_lsb;
	uint8_t param_msb;
};

struct read_package_res_t {
	uint16_t cmd;
	uint16_t rsvd;
	uint8_t peci_err_satus;
	uint8_t peci_completeion_code;
	uint8_t data[4];
};

static int peci_get_temp(uint8_t *temp)
{
	int ret;
	int16_t raw_temp;
	struct get_temp_res_t *res;
	struct get_temp_req_t get_temp = {
		.cmd = PMC_PECI_GET_TEMP_CMD,
		.rsvd = 0x0,
		.peci_adr = PECI_TARGET_ADDRESS,
		.wr_len = 0x1,
		.rd_len = 0x2,
		.peci_cmd = PECI_GET_TEMP_CMD,
	};

	LONG_MSG_HEADER(PMC_WAIT_ACK, PMC_MSG_HEADER_SIZE +
			sizeof(struct get_temp_req_t));
	memcpy(&peci_msg.u.msg[0], &get_temp, sizeof(struct get_temp_req_t));

	ret = pmc_sync_send_msg(&peci_msg);
	res = (struct get_temp_res_t *)(&peci_msg.u.msg[0]);

	if (ret) {
		LOG_ERR("Get Temp Error: %d", ret);
		return ret;
	}
	/* raw_temp hold temprature value in 2's complement format. BIT15 to
	 * BIT6 hold integer part.
	 */
	raw_temp = res->res_lsb | (res->res_msb << 8);
	*temp = TJ_MAX + (raw_temp >> RAW_TO_INT);
	ECLITE_LOG_DEBUG("CPU temp: %d, raw_temp: %d, TJ_MAX: %d",
			 *temp, raw_temp, TJ_MAX);

	return 0;
}

static int peci_read_package(struct read_package_req_t *req, uint8_t reqlen,
			     uint8_t reslen,
			     struct read_package_res_t *res)
{
	int ret;

	LONG_MSG_HEADER(PMC_WAIT_ACK, PMC_MSG_HEADER_SIZE + reqlen);
	memcpy(&peci_msg.u.msg[0], (uint8_t *)req, reqlen);

	ret = pmc_sync_send_msg(&peci_msg);

	/* report received data irrespective of error.*/
	memcpy((uint8_t *)res, &peci_msg.u.msg[0], reslen);

	if (ret) {
		LOG_ERR("PMC RdPkg failed: %d", ret);
		return ret;
	}

	if (res->peci_completeion_code != COMPLETION_CODE) {
		LOG_ERR("PMC RdPkg completion failed");
		return ERROR;
	}

	return 0;
}

static int get_tj_max(uint8_t *out)
{
	int ret;
	struct read_package_req_t msg_in;
	struct read_package_res_t msg_out;

	msg_in.cmd = PMC_PECI_RD_PKG_CMD;
	msg_in.rsvd = 0x0;
	msg_in.peci_add = PECI_TARGET_ADDRESS;
	msg_in.wr_len = 0x5;
	msg_in.rd_len = 0x5;
	msg_in.peci_cmd = PECI_RD_PKG_CMD;
	msg_in.host_id_retry = 0x0;
	msg_in.index = PECI_TJMAX_INDEX;
	msg_in.param_lsb = 0x0;
	msg_in.param_msb = 0x0;

	ret = peci_read_package(&msg_in, msg_in.wr_len + REQ_LEN,
				msg_in.rd_len + RESP_LEN, &msg_out);

	if (ret) {
		*out = PECI_FAIL_SAFE_TJMAX;
		LOG_ERR("PECI Get TJMAX failed");
		return ret;
	}

	/* TjMax*/
	*out = msg_out.data[2];

	return 0;
}

static int pmc_send_short_msg(uint8_t *buf)
{
	int ret;

	SHORT_MSG_HEADER(PMC_NO_WAIT_ACK);
	peci_msg.u.short_msg.cmd_id = PMC_POWER_STATE_CHANGE_CMD;
	peci_msg.u.short_msg.payload = *buf;

	ret = pmc_sync_send_msg(&peci_msg);

	if (ret) {
		LOG_ERR("Short msg error");
		return ret;
	}

	return 0;
}

int pmc_command(uint8_t msg_type, uint8_t *buf)
{
	int ret;

	switch (msg_type) {
	case GET_TEMP:
		ret = peci_get_temp(buf);
		break;
	case GET_TJ_MAX:
		ret = get_tj_max(buf);
		tj_max = *buf;
		break;
	case SEND_POWER_STATE_CHANGE:
		ret = pmc_send_short_msg(buf);
		break;
	default:
		LOG_ERR("Invalid PMC request");
		ret = ERROR;
		break;
	}
	LOG_INF("msg_type: %d, data: %d", msg_type, *buf);
	return ret;
}

/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(sys_mng, CONFIG_HECI_LOG_LEVEL);

#include <init.h>
#include <host_ipc_service.h>
#include <ipc_helper.h>
#include "driver/sedi_driver_rtc.h"

#define MNG_RX_CMPL_ENABLE       0
#define MNG_RX_CMPL_DISABLE      1
#define MNG_RX_CMPL_INDICATION   2
#define MNG_RESET_NOTIFY         3
#define MNG_RESET_NOTIFY_ACK     4
#define MNG_TIME_UPDATE          5
#define MNG_RESET_REQUEST        6
#define MNG_ILLEGAL_CMD          0xFF

#define HOST_COMM_REG 0x40400038
#define HOST_RDY_BIT 7
#define IS_HOST_UP(host_comm_reg) (host_comm_reg & BIT(HOST_RDY_BIT))

#define MIN_RESET_INTV 100000

bool rx_complete_enabled = true;
bool rx_complete_changed;
uint64_t last_reset_notify_time;
struct ipc_mng_playload_tpye {
	uint16_t reset_id;
	uint16_t capabilities;
};

static int send_reset_to_peer(const struct device *dev, uint32_t command,
			      uint16_t reset_id)
{
	int ret;
	uint64_t current_time;
	struct ipc_mng_playload_tpye ipc_mng_msg = {
		.reset_id = reset_id,
		.capabilities = MNG_CAP_SUPPORTED
	};

	uint32_t drbl = IPC_BUILD_MNG_DRBL(command, sizeof(ipc_mng_msg));
	sedi_rtc_get_us(&current_time);

	LOG_DBG("");
	if ((last_reset_notify_time != 0)
	    && (current_time - last_reset_notify_time <  MIN_RESET_INTV)
	    && (command == MNG_RESET_NOTIFY)) {
		LOG_WRN("interval is too short, won't send");
		return -1;
	}

	ret = ipc_write_msg(dev, drbl, (uint8_t *)&ipc_mng_msg,
			    sizeof(ipc_mng_msg), NULL, NULL, 0);
	if (command == MNG_RESET_NOTIFY) {
		last_reset_notify_time = current_time;
	}
	return ret;
}

int send_rx_complete(const struct device *dev)
{
	int ret = 0;
	uint32_t rx_comp_drbl = IPC_BUILD_MNG_DRBL(MNG_RX_CMPL_INDICATION, 0);

	if (rx_complete_enabled) {
		ret = ipc_write_msg(dev, rx_comp_drbl, NULL, 0, NULL, NULL, 0);
		if (ret) {
			LOG_ERR("fail to send rx_complete msg");
		}
	}

	return ret;
}

static int sys_mng_handler(const struct device *dev, uint32_t drbl)
{
	int cmd = IPC_HEADER_GET_MNG_CMD(drbl);
	uint32_t drbl_ack = drbl & (~BIT(IPC_DRBL_BUSY_OFFS));

	struct ipc_mng_playload_tpye mng_msg;

	LOG_DBG("ipc received a management msg, drbl = %08x", drbl);

	ipc_read_msg(dev, NULL, (uint8_t *)&mng_msg, sizeof(mng_msg));
	ipc_send_ack(dev, drbl_ack, NULL, 0);
	send_rx_complete(dev);

	switch (cmd) {
	case MNG_RX_CMPL_ENABLE:
		rx_complete_enabled = true;
		rx_complete_changed = true;
		break;
	case MNG_RX_CMPL_DISABLE:
		rx_complete_enabled = false;
		rx_complete_changed = true;
		break;
	case MNG_RX_CMPL_INDICATION:
		/* not used yet */
		break;
	case MNG_RESET_NOTIFY:
#if CONFIG_HECI
		heci_reset();
#endif
		send_reset_to_peer(dev, MNG_RESET_NOTIFY_ACK, mng_msg.reset_id);
	case MNG_RESET_NOTIFY_ACK:
		sedi_fwst_set(ILUP_HOST, 1);
		sedi_fwst_set(HECI_READY, 1);
		LOG_DBG("ipc link is up");
		break;
	case MNG_TIME_UPDATE:
		LOG_DBG("no time update support");
		break;
	case MNG_RESET_REQUEST:
		LOG_DBG("host requests pse to reset, not support, do nothing");
		break;
	default:
		LOG_ERR("invaild sysmng ipc cmd, cmd = %02x", cmd);
		return -1;
	}
	return 0;
}

static int sys_boot_handler(const struct device *dev, uint32_t drbl)
{
	ipc_send_ack(dev, 0, NULL, 0);
	send_rx_complete(dev);
	if (drbl == BIT(IPC_DRBL_BUSY_OFFS)) {
		send_reset_to_peer(dev, MNG_RESET_NOTIFY, 0);
	}

	return 0;
}

int mng_and_boot_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int ret;
	const struct device *dev_ipc = device_get_binding("IPC_HOST");

	ret = host_protocol_register(IPC_PROTOCOL_MNG, sys_mng_handler);
	if (ret != 0) {
		LOG_ERR("fail to add sys_mng_handler as cb fun");
		return -1;
	}
	ret = host_protocol_register(IPC_PROTOCOL_BOOT, sys_boot_handler);
	if (ret != 0) {
		LOG_ERR("fail to add sys_boot_handler as cb fun");
		return -1;
	}
	LOG_DBG("register system ipc message handler successfully");

	if (IS_HOST_UP(read32(HOST_COMM_REG))) {
		send_reset_to_peer(dev_ipc, MNG_RESET_NOTIFY, 0);
	}
	return 0;
}

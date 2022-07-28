/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(sys_mng, CONFIG_HECI_LOG_LEVEL);

#include <kernel.h>
#include <init.h>
#include <kernel.h>
#include <host_ipc_service.h>
#include <ipc_helper.h>
#include <pm_service.h>
#include <user_app_framework/user_app_framework.h>
#include <user_app_framework/user_app_config.h>
#include "driver/sedi_driver_rtc.h"
#include "driver/sedi_driver_pm.h"
#if CONFIG_HOST_TIME_SYNC
#include "driver/sedi_driver_tsync.h"
#include <posix/time.h>
#endif

#define MNG_RX_CMPL_ENABLE       0
#define MNG_RX_CMPL_DISABLE      1
#define MNG_RX_CMPL_INDICATION   2
#define MNG_RESET_NOTIFY         3
#define MNG_RESET_NOTIFY_ACK     4
#define MNG_TIME_UPDATE          5
#define MNG_RESET_REQUEST        6
#define MNG_RTD3_NOTIFY          7
#define MNG_RTD3_NOTIFY_ACK      8
#define MNG_D0_NOTIFY            9
#define MNG_D0_NOTIFY_ACK        10

#define MNG_ILLEGAL_CMD          0xFF
#define MAX_MNG_MSG_LEN          128

#define HOST_COMM_REG 0x40400038
#define HOST_RDY_BIT 7
#define IS_HOST_UP(host_comm_reg) (host_comm_reg & BIT(HOST_RDY_BIT))

#define MIN_RESET_INTV 100000

#define DSTATE_0                0
#define DSTATE_RTD3_NOTIFIED    1
#define DSTATE_RTD3             2

bool rx_complete_enabled = true;
bool rx_complete_changed;
uint64_t last_reset_notify_time;

K_SEM_DEFINE(sem_d3, 0, 1);
K_SEM_DEFINE(sem_rtd3, 1, 1);
K_MUTEX_DEFINE(rtd3_map_lock);

APP_SHARED_VAR atomic_t is_waiting_d3 = ATOMIC_INIT(0);
APP_SHARED_VAR uint32_t rtd3_req_map;

struct reset_payload_tpye {
	uint16_t reset_id;
	uint16_t capabilities;
};

static int mng_host_req_d0(int32_t timeout);
/*!
 * \fn int mng_host_access_req()
 * \brief request access to host
 * \param[in] timeout: timeout time
 * \return 0 or error codes
 */
APP_SHARED_VAR int (*mng_host_access_req)(int32_t /* timeout */) =
	mng_host_req_d0;

void mng_host_access_dereq(void);

int host_access_req(int32_t timeout)
{
	int ret;

	ret = k_mutex_lock(&rtd3_map_lock, K_MSEC(timeout));
	if (ret) {
		LOG_ERR("error when requesting rtd3, occupied");
		return ret;
	}
	if (rtd3_req_map == UINT32_MAX) {
		LOG_ERR("error when requesting rtd3");
		k_mutex_unlock(&rtd3_map_lock);
		return -1;
	}
	if (rtd3_req_map == 0) {
		ret = mng_host_access_req(timeout);
		if (ret) {
			LOG_DBG("host access requested failed");
			k_mutex_unlock(&rtd3_map_lock);
			return ret;
		}
	}
	ret = find_lsb_set(rtd3_req_map);
	rtd3_req_map |= BIT(ret);
	k_mutex_unlock(&rtd3_map_lock);
	return ret;
}

int host_access_dereq(int handler)
{
	k_mutex_lock(&rtd3_map_lock, K_FOREVER);
	if ((handler >= 0) && (handler <= 31) && (rtd3_req_map & BIT(handler))) {
		rtd3_req_map &= (~BIT(handler));
		if (rtd3_req_map == 0) {
			mng_host_access_dereq();
		}
		k_mutex_unlock(&rtd3_map_lock);
		return 0;
	}
	LOG_ERR("invaild host dereq argument");
	k_mutex_unlock(&rtd3_map_lock);
	return -1;
}

static int mng_host_req_d0(int32_t timeout)
{
	return k_sem_take(&sem_rtd3, K_MSEC(timeout));
}

static int mng_host_req_rtd3(int32_t timeout)
{
	power_trigger_pme(PSE_DEV_LHIPC);
	LOG_DBG("PME wake is triggered\n");
	return k_sem_take(&sem_rtd3, K_MSEC(timeout));
}

/*!
 * \fn mng_host_req_sx
 * \brief called by status switching from rtd3_warning to Sx
 * \will reset all related variables to D0 state.
 * \param[in] timeout: timeout time
 * \return RTD3_SWITCH_TO_SX as HECI send can not going on.
 */
static int mng_host_req_sx(int32_t timeout)
{	/* reset RTD3 related variables to D0 state */
	mng_host_access_req = mng_host_req_d0;
	LOG_DBG("Switched to Sx status!\n");
	return RTD3_SWITCH_TO_SX;
}

static int mng_host_req_rtd3_notified(int32_t timeout)
{
	LOG_DBG("RTD3 notified state\n");
	atomic_inc(&is_waiting_d3);
	k_sem_take(&sem_d3, K_MSEC(timeout));
	if (mng_host_access_req == mng_host_req_rtd3_notified) {
		atomic_set(&is_waiting_d3, 0);
		LOG_DBG("Failed to get out RTD3_notified state in %d ms!\n",
			timeout);
		return RTD3_NOTIFIED_STUCK;
	} else {
		return mng_host_access_req(timeout);
	}
}

#if CONFIG_HOST_TIME_SYNC

/* Host and PMC have same frequency */
#define TSYNC_HOST_LOCAL_MULTI TSYNC_PMC_LOCAL_MULTI

/* indication for the field sequence of host utc and system time in message */
#define TFMT_SYSTEM_TIME          1
#define TFMT_ART_TIME             2

struct host_clock_data {
	uint64_t primary_host_time;
	struct {
		uint8_t primary_source;
		uint8_t secondary_source;
		uint16_t reserved;
	} time_format;
	uint64_t secondary_host_time;
} __packed;

static void handle_host_time_sync(uint8_t *data, uint8_t data_len)
{
	struct host_clock_data *const sync_data =
			(struct host_clock_data *const)data;
	uint64_t tsync_cnt;
	uint64_t host_utc_us;
	uint64_t host_art_cnt;
	uint64_t trans_us;
	struct timespec tp;
#ifdef CONFIG_HECI_LOG_LEVEL_DBG
	struct timespec tp_fw;
	uint64_t fw_utc_us;
	static uint64_t last_sync_us;
#endif

	if ((data_len != sizeof(struct host_clock_data)) ||
		(sync_data->time_format.primary_source != TFMT_ART_TIME)) {
		LOG_DBG("Unknown time sync format, len=%u, FMT=%u", data_len,
			(data_len == sizeof(struct host_clock_data)) ?
			sync_data->time_format.primary_source : (uint32_t)-1);
		return;
	}

	host_art_cnt = sync_data->primary_host_time / TSYNC_HOST_LOCAL_MULTI;
	host_utc_us = sync_data->secondary_host_time;

	sedi_tsync_sync();
	sedi_tsync_get_time(&tsync_cnt);

	if (tsync_cnt < host_art_cnt) {
		LOG_ERR("Wrong time sync data, host ART time is "
				"bigger than FW TSYNC time");
		return;
	}

	/* time used to pass down here */
	trans_us = (tsync_cnt - host_art_cnt) * USEC_PER_SEC /
			TSYNC_DEFAULT_FREQ;
	host_utc_us += trans_us;

#ifdef CONFIG_HECI_LOG_LEVEL_DBG
	clock_gettime(CLOCK_REALTIME, &tp_fw);
#endif

	tp.tv_sec = host_utc_us / USEC_PER_SEC;
	tp.tv_nsec = (host_utc_us % USEC_PER_SEC) * NSEC_PER_USEC;
	clock_settime(CLOCK_REALTIME, &tp);

#ifdef CONFIG_HECI_LOG_LEVEL_DBG
	fw_utc_us = ((uint64_t)tp_fw.tv_sec * USEC_PER_SEC) +
			((uint64_t)tp_fw.tv_nsec / NSEC_PER_USEC);

	LOG_DBG("Sync with Host: %+d after %u us",
			(int32_t)(host_utc_us - fw_utc_us),
			(uint32_t)(host_utc_us - last_sync_us));
	last_sync_us = host_utc_us;
	LOG_DBG(" Host UTC Time: (0x%x, 0x%x)",
			(uint32_t)(host_utc_us >> 32),
			(uint32_t)host_utc_us);
#endif
}

#endif

static int send_reset_to_peer(const struct device *dev, uint32_t command,
			      uint16_t reset_id)
{
	int ret;
	uint64_t current_time;
	struct reset_payload_tpye ipc_mng_msg = {
		.reset_id = reset_id,
		.capabilities = MNG_CAP_SUPPORTED
	};

	uint32_t drbl = IPC_BUILD_MNG_DRBL(command, sizeof(ipc_mng_msg));

	sedi_rtc_get_us(&current_time);

	LOG_DBG("");
	if ((last_reset_notify_time != 0)
	    && (current_time - last_reset_notify_time <  MIN_RESET_INTV)
	    && (command == MNG_RESET_NOTIFY)) {
		LOG_WRN("interval is too short, won't send\n");
		return -1;
	}

	ret = ipc_write_msg(dev, drbl, (uint8_t *)&ipc_mng_msg,
			    sizeof(ipc_mng_msg), NULL, NULL, 0);
	if (command == MNG_RESET_NOTIFY) {
		last_reset_notify_time = current_time;
	}
	return ret;
}

void mng_sx_entry(void)
{
	if (atomic_set(&is_waiting_d3, 0)) {
		/* There's thread blocking at mng_host_access_req
		 * after RTD3_notify is received.
		 */
		mng_host_access_req = mng_host_req_sx;
		k_sem_give(&sem_d3);
	} else {
		mng_host_access_req = mng_host_req_d0;
	}
	k_sem_give(&sem_rtd3);
}

int send_rx_complete(const struct device *dev)
{
	int ret = 0;
	uint32_t rx_comp_drbl = IPC_BUILD_MNG_DRBL(MNG_RX_CMPL_INDICATION, 0);

	if (rx_complete_enabled) {
		ret = ipc_write_msg(dev, rx_comp_drbl, NULL, 0, NULL, NULL, 0);
		if (ret) {
			LOG_ERR("fail to send rx_complete msg\n");
		}
	}

	return ret;
}

static uint8_t mng_in_msg[MAX_MNG_MSG_LEN];
static int sys_mng_handler(const struct device *dev, uint32_t drbl)
{
	int cmd = IPC_HEADER_GET_MNG_CMD(drbl);
	uint32_t drbl_ack = drbl & (~BIT(IPC_DRBL_BUSY_OFFS));

	struct reset_payload_tpye *rst_msg;

	LOG_DBG("received a management msg, drbl = %08x", drbl);
	__ASSERT(IPC_HEADER_GET_LENGTH(drbl) <= MAX_MNG_MSG_LEN, "bad mng msg");

	ipc_read_msg(dev, NULL, (uint8_t *)&mng_in_msg, sizeof(mng_in_msg));
	ipc_send_ack(dev, drbl_ack, NULL, 0);
	send_rx_complete(dev);

	LOG_HEXDUMP_DBG(mng_in_msg, IPC_HEADER_GET_LENGTH(drbl), "mng incoming");
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
	case MNG_D0_NOTIFY:
		LOG_DBG("\nD0 warning received!\n");
		mng_host_access_req = mng_host_req_d0;
		sedi_pm_set_hostipc_event(SEDI_PM_HOSTIPC_D0_NOTIFY);   /* 2 for D0 */
		ipc_write_msg(dev, IPC_BUILD_MNG_DRBL(MNG_D0_NOTIFY_ACK, 0),
			      NULL, 0, NULL, NULL, 0);
		break;

	case MNG_RESET_NOTIFY:
#if CONFIG_HECI
		heci_reset();
#endif
		rst_msg = (struct reset_payload_tpye *)mng_in_msg;
		send_reset_to_peer(dev, MNG_RESET_NOTIFY_ACK, rst_msg->reset_id);
	case MNG_RESET_NOTIFY_ACK:
		sedi_fwst_set(ILUP_HOST, 1);
		sedi_fwst_set(HECI_READY, 1);
		LOG_DBG("ipc link is up\n");
		break;
	case MNG_TIME_UPDATE:
#if CONFIG_HOST_TIME_SYNC
		handle_host_time_sync(mng_in_msg,
				      IPC_HEADER_GET_LENGTH(drbl));
#else
		LOG_DBG("no time update support\n");
#endif
		break;
	case MNG_RESET_REQUEST:
		LOG_DBG("host requests pse to reset, not support, do nothing");
		break;
	case MNG_RTD3_NOTIFY:
		LOG_DBG("\nRTD3 warning received!\n");
		int rtd3_ready = !(k_sem_take(&sem_rtd3, K_NO_WAIT));

		if (rtd3_ready) {
			sedi_pm_set_hostipc_event(SEDI_PM_HOSTIPC_RTD3_NOTIFY);
			mng_host_access_req = mng_host_req_rtd3_notified;
		}
		ipc_write_msg(dev, IPC_BUILD_MNG_DRBL(MNG_RTD3_NOTIFY_ACK,
						      sizeof(uint32_t)),
			      (uint8_t *)&rtd3_ready,
			      0, NULL, NULL, 0);
		break;

	default:
		LOG_ERR("invaild sysmng ipc cmd, cmd = %02x\n", cmd);
		return -1;
	}
	return 0;
}

static int sys_boot_handler(const struct device *dev, uint32_t drbl)
{
	ipc_send_ack(dev, 0, NULL, 0);
	send_rx_complete(dev);
	if (drbl == BIT(IPC_DRBL_BUSY_OFFS)) {
#if CONFIG_HECI
		heci_reset();
#endif
		send_reset_to_peer(dev, MNG_RESET_NOTIFY, 0);
	}

	return 0;
}

static void mng_d3_proc(sedi_pm_d3_event_t d3_event, void *ctx)
{
	ARG_UNUSED(ctx);

	switch (d3_event) {
	case PM_EVENT_HOST_RTD3_ENTRY:
		mng_host_access_req = mng_host_req_rtd3;
		if (atomic_set(&is_waiting_d3, 0)) {
			k_sem_give(&sem_d3);
		}
		LOG_DBG("\nRTD3_ENTRY received!\n");
		break;

	case PM_EVENT_HOST_RTD3_EXIT:
		mng_host_access_req = mng_host_req_d0;
		LOG_DBG("\nRTD3_EXIT received!\n");
		break;
	default:
		break;
	}
}


void mng_host_access_dereq(void)
{
	k_sem_give(&sem_rtd3);
}

int mng_and_boot_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int ret;
	const struct device *dev_ipc = device_get_binding("IPC_HOST");

	sedi_pm_register_d3_notification(PSE_DEV_LHIPC, mng_d3_proc, NULL);

	ret = host_protocol_register(IPC_PROTOCOL_MNG, sys_mng_handler);
	if (ret != 0) {
		LOG_ERR("fail to add sys_mng_handler as cb fun\n");
		return -1;
	}
	ret = host_protocol_register(IPC_PROTOCOL_BOOT, sys_boot_handler);
	if (ret != 0) {
		LOG_ERR("fail to add sys_boot_handler as cb fun\n");
		return -1;
	}
	LOG_DBG("register system ipc message handler successfully\n");

	if (IS_HOST_UP(read32(HOST_COMM_REG))) {
		send_reset_to_peer(dev_ipc, MNG_RESET_NOTIFY, 0);
	}
	return 0;
}

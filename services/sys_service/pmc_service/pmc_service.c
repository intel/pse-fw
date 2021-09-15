/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel_structs.h>
#include <device.h>
#include <drivers/ipc.h>
#include <sys/printk.h>
#include <sys/reboot.h>
#include <string.h>
#include <user_app_framework/user_app_framework.h>
#include <user_app_framework/user_app_config.h>
#include "pmc_service.h"
#include "sedi.h"
#include "pmc_service_common.h"

#define LOG_LEVEL CONFIG_PMC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(pmc);

#if !defined(CONFIG_SYS_SERVICE)
#error PMC Service require SYS service to be enabled!! \
	Please config CONFIG_SYS_SERVICE = y
#endif
#define PMC_MSG_THREAD_PRIORITY 10
#define PMC_MSG_TASK_STACK_SIZE (1600)
#define PMC_IPC_READ_TIMEOUT (5000)
#define PMC_IPC_BUSY (1)
#define SB_DEV "SIDEBAND"
#define DB_FORMAT_SHORT (1)

struct pmc_msg_intrnl_t {
	void *resv;
	uint32_t status;
	struct pmc_msg_t usr_msg;
};

typedef union _ipc_drbl_t {
	struct {
		uint32_t payload_size : 10;
		uint32_t client_id : 6;
		uint32_t resv1 : 3;
		uint32_t long_frmt_ver : 5;
		uint32_t resv2 : 2;
		uint32_t flow_ctrl : 1;
		uint32_t short_frmt : 1;
		uint32_t custom_frmt : 1;
		uint32_t resv3 : 2;
		uint32_t busy : 1;
	} db;
	uint32_t val;
} ipc_drbl_t;

typedef union _ipc_drbl_short_t {
	struct {
		uint32_t payload : 8;
		uint32_t client_id : 6;
		uint32_t cmd_id : 8;
		uint32_t short_frmt_ver : 4;
		uint32_t flow_ctrl : 1;
		uint32_t short_frmt : 1;
		uint32_t custom_frmt : 1;
		uint32_t resv3 : 2;
		uint32_t busy : 1;
	} db;
	uint32_t val;
} ipc_drbl_short_t;

__kernel struct k_thread pmc_msg_thread_handle;

K_THREAD_STACK_DEFINE(pmc_msg_thread_stack, PMC_MSG_TASK_STACK_SIZE);

K_FIFO_DEFINE(pmc_serv_fifo);

K_MUTEX_DEFINE(pmc_msg_mutex);
K_SEM_DEFINE(pmc_msg_alert, 0, 1);
K_SEM_DEFINE(ipc_alert, 0, 1);

#if !defined(CONFIG_RUN_PMC_SERV_SUPERVISORY)
APP_GLOBAL_VAR(0) static struct k_thread *app_handle[CONFIG_PMC_MAX_CLIENT];
#endif

APP_GLOBAL_VAR(0) static const struct device *sb_dev;

#if !defined(CONFIG_PMC_DUMMY_RESP)
APP_GLOBAL_VAR(0) const struct device *ipc_dev;

static int ipc_rx_handler(const struct device *dev, void *arg)
{
	k_sem_give(&ipc_alert);
	return 0;
}

static int send_pmc_ipc_msg(struct pmc_msg_t *usr_msg)
{
	LOG_DBG("%s\n", __func__);
	int ret;
	ipc_drbl_t drbl;
	ipc_drbl_short_t drbl_short;

	if (usr_msg->format == FORMAT_LONG) {
		drbl.val = 0;
		drbl.db.busy = PMC_IPC_BUSY;
		drbl.db.payload_size = usr_msg->msg_size;
		LOG_DBG("Sending PMC Long Msg Doorbell:%x\n", (uint32_t)drbl.val);
		ret = ipc_write_msg(ipc_dev, drbl.val,
						 usr_msg->u.msg,
						 usr_msg->msg_size, NULL,
						 NULL, 0);
		if (ret) {
			LOG_ERR("IPC write message timeout error: %d\n", ret);
		}
	} else if (usr_msg->format == FORMAT_SHORT) {
		drbl_short.val = 0;
		drbl_short.db.short_frmt = DB_FORMAT_SHORT;
		drbl_short.db.busy = PMC_IPC_BUSY;
		drbl_short.db.cmd_id = usr_msg->u.short_msg.cmd_id;
		drbl_short.db.payload = usr_msg->u.short_msg.payload;
		drbl_short.db.short_frmt_ver = 1;
		LOG_DBG("Sending Short PMC Msg Doorbell:%x\n",
			(uint32_t)drbl_short.val);
		ret = ipc_write_msg(ipc_dev, drbl_short.val, NULL,
						 0, NULL, NULL, 0);
		if (ret) {
			LOG_ERR("IPC write message timeout error: %d\n", ret);
		}
	} else if (usr_msg->format == FORMAT_SB_RAW_WRITE) {
		LOG_DBG("Sending Raw write message\n");
		ret = sideband_write_raw(sb_dev, &usr_msg->u.raw_msg);
	} else if (usr_msg->format == FORMAT_SB_RAW_READ) {
		LOG_DBG("Sending Raw read message\n");
		ret = sideband_read_raw(sb_dev, &usr_msg->u.raw_msg);
	} else if (usr_msg->format == FORMAT_HWSB_PME_REQ) {
		LOG_DBG("Sending HW initiated PME SB message\n");
		ret = sedi_pm_trigger_pme(PSE_DEV_LHIPC);
	} else if (usr_msg->format == FORMAT_WIRE_GLOBAL_RESET) {
		LOG_DBG("Trigger wired global reset");
		ret = sedi_pm_enable_global_reset(SW_RESET, true);
		sys_reboot(SYS_REBOOT_WARM);
	} else {
		LOG_ERR("eror ipc_write_msg\n");
		return -EINVAL;
	}

	LOG_DBG(" status:%x\n", ret);
	return ret;
}

static int rcv_pmc_ipc_msg(struct pmc_msg_t *usr_msg)
{
	LOG_DBG("%s\n", __func__);
	int ret;
	ipc_drbl_t drbl;

	ret = k_sem_take(&ipc_alert, K_MSEC(PMC_IPC_READ_TIMEOUT));
	if (ret) {
		LOG_ERR("PMC Rx Timed Out\n");
		return ret;
	}
	if (usr_msg->format == FORMAT_LONG) {
		drbl.val = 0;
		ret = ipc_read_msg(ipc_dev, &drbl.val, usr_msg->u.msg,
				   usr_msg->msg_size);
		LOG_DBG("Received Long Msg Doorbell:%x\n", (uint32_t)drbl.val);
	} else if (usr_msg->format == FORMAT_SHORT) {
		ret = ipc_read_msg(ipc_dev, &usr_msg->u.short_msg.drbl_val,
				   NULL, 0);
	} else {
		LOG_ERR("error ipc_read_msg\n");
		return -EINVAL;
	}

	ipc_send_ack(ipc_dev, 0, NULL, 0);

	LOG_DBG("status:%x\n", ret);
	return ret;
}

#endif

int pmc_sync_send_msg(struct pmc_msg_t *usr_msg)
{
	LOG_DBG("%s\n", __func__);
	int ret;
	struct pmc_msg_intrnl_t pmc_msg;

	if (!usr_msg || usr_msg->client_id >= CONFIG_PMC_MAX_CLIENT ||
	    usr_msg->msg_size >= MAX_PECI_MSG_SIZE) {
		LOG_ERR("Invalid Arugment given\n");
		return -EINVAL;
	}

	k_mutex_lock(&pmc_msg_mutex, K_FOREVER);

	memcpy(&pmc_msg.usr_msg, usr_msg, sizeof(struct pmc_msg_t));

	pmc_msg.status = 0;

	ret = k_fifo_alloc_put(&pmc_serv_fifo, &pmc_msg);
	if (ret) {
		LOG_ERR("PMC Fifo allocation failed");
		goto err_exit;
	}

	k_sem_take(&pmc_msg_alert, K_FOREVER);

	ret = pmc_msg.status;
	memcpy(usr_msg, &pmc_msg.usr_msg, sizeof(struct pmc_msg_t));

err_exit:
	k_mutex_unlock(&pmc_msg_mutex);

	return ret;
}

static void pmc_msg_thread(void *p1, void *p2, void *p3)
{
	struct pmc_msg_intrnl_t *pmc_msg;
	struct pmc_msg_t *usr_msg;

	LOG_DBG("%s\n", __func__);

	while (1) {
		pmc_msg = k_fifo_get(&pmc_serv_fifo, K_FOREVER);
		if (!pmc_msg) {
			LOG_ERR("Error while retrieve pmc msg from client\n");
			continue;
		} else {
			LOG_DBG("pmc msg request from Client:%d\n",
				pmc_msg->usr_msg.client_id);
			usr_msg = &pmc_msg->usr_msg;
#if !defined(CONFIG_PMC_DUMMY_RESP)
			pmc_msg->status = send_pmc_ipc_msg(usr_msg);
			if (pmc_msg->status == 0 &&
			    usr_msg->wait_for_ack == PMC_WAIT_ACK) {
				pmc_msg->status = rcv_pmc_ipc_msg(usr_msg);
			}
#endif
		}

		k_sem_give(&pmc_msg_alert);
	}
}

void pmc_config(void)
{
	struct user_app_res_table_t *res_table;

	LOG_DBG("%s\n", __func__);

	sb_dev = device_get_binding(SB_DEV);

	if (!sb_dev) {
		LOG_DBG("Cannot get sideband device.\n");
		return;
	}

#if !defined(CONFIG_PMC_DUMMY_RESP)
	ipc_dev = device_get_binding("IPC_PMC");

	if (!ipc_dev) {
		LOG_ERR("unable to get IPC driver handle:%s\n",
			"IPC_PMC");
		return;
	}

	ipc_set_rx_notify(ipc_dev, ipc_rx_handler);
#endif

#if defined(CONFIG_RUN_PMC_SERV_SUPERVISORY)
	k_thread_create(&pmc_msg_thread_handle, pmc_msg_thread_stack,
			PMC_MSG_TASK_STACK_SIZE, pmc_msg_thread, NULL, NULL,
			NULL, PMC_MSG_THREAD_PRIORITY, 0, K_FOREVER);

	for (int i = 0; i < CONFIG_NUM_USER_APP_AND_SERVICE; i++) {
		res_table = get_user_app_res_table(i);
		if (res_table) {
			LOG_DBG("app_handle%d:%p\n", i, res_table->app_handle);
			if (res_table->app_handle) {
				LOG_DBG("grand PMC access to App:%d\n", i);
				k_thread_access_grant(
					res_table->app_handle, &pmc_serv_fifo,
					&pmc_msg_alert, &pmc_msg_mutex, NULL);
			}
		}
	}
	k_thread_start(&pmc_msg_thread_handle);
#else
	for (int i = 0; i < CONFIG_NUM_USER_APP_AND_SERVICE; i++) {
		res_table = get_user_app_res_table(i);
		for (int j = 0; j < res_table->dev_cnt; j++) {
			LOG_DBG("App:%d, sys_service:%x, dev_list[%d/%d]:%s\n",
				res_table->app_id, res_table->sys_service, j,
				res_table->dev_cnt, log_strdup(res_table->dev_list[j]));
		}
		if (res_table->sys_service & PMC_SERVICE) {
			app_handle[i] = res_table->app_handle;
			LOG_DBG("app_handle:%x\n", app_handle[i]);
		}
	}
#endif
}

void pmc_service_init(void)
{
	LOG_DBG("%s\n", __func__);

#if !defined(CONFIG_RUN_PMC_SERV_SUPERVISORY)
	LOG_DBG("Create pmc msg thread\n");
	k_thread_create(&pmc_msg_thread_handle, pmc_msg_thread_stack,
			PMC_MSG_TASK_STACK_SIZE, pmc_msg_thread, NULL, NULL,
			NULL, 10, APP_USER_MODE_TASK | K_INHERIT_PERMS,
			K_FOREVER);

	for (int i = 0; i < CONFIG_PMC_MAX_CLIENT; i++) {
		LOG_DBG("app_handle:%x\n", app_handle[i]);
		if (app_handle[i]) {
			LOG_DBG("grand PMC access to App:%d\n", i);
			k_thread_access_grant(app_handle[i], &pmc_serv_fifo,
					      &pmc_msg_alert, &pmc_msg_mutex,
					      NULL);
		}
	}
	k_thread_start(&pmc_msg_thread_handle);
#endif
}

/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <device.h>
#include <kernel.h>
#include <user_app_framework/user_app_framework.h>
#include <user_app_framework/user_app_config.h>
#include <host_ipc_service.h>
#include <ipc_helper.h>
#include "host_service_common.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(host, CONFIG_IPC_LOG_LEVEL);

#define MAX_SERVICE_CLIENTS 16

#define SERVICE_STACK_SIZE 1600
__kernel struct k_thread host_service_thread;
K_SEM_DEFINE(sem_ipc_read, 0, 1)

K_THREAD_STACK_DEFINE(host_service_stack, SERVICE_STACK_SIZE);
static ipc_msg_handler_f protocol_cb[MAX_SERVICE_CLIENTS] = { 0 };

extern __kernel struct k_mutex dev_lock;
extern __kernel struct k_sem sem_rtd3;
extern __kernel struct k_sem sem_d3;
extern __kernel struct k_mutex rtd3_map_lock;

int host_protocol_register(uint8_t protocol_id, ipc_msg_handler_f handler)
{
	if ((handler == NULL) || (protocol_id >= MAX_SERVICE_CLIENTS)) {
		LOG_ERR("bad params");
		return -1;
	}

	if (protocol_cb[protocol_id] != NULL) {
		LOG_WRN("host protocol registered already");
		return -1;
	}
	protocol_cb[protocol_id] = handler;
	LOG_INF("add ipc handler function, protocol_id=%d", protocol_id);
	return 0;
}

#define MNG_D0_NOTIFY            9

static int ipc_rx_handler(const struct device *dev, void *arg)
{
	uint32_t inbound_drbl;
	int cmd;
	uint8_t protocol;

	ipc_read_drbl(dev, &inbound_drbl);
	cmd = IPC_HEADER_GET_MNG_CMD(inbound_drbl);
	protocol =  IPC_HEADER_GET_PROTOCOL(inbound_drbl);

	k_sem_give(&sem_ipc_read);
	if ((protocol == IPC_PROTOCOL_MNG) && (cmd == MNG_D0_NOTIFY)) {
		k_sem_give(&sem_rtd3);
	}
	return 0;
}

static void ipc_rx_task(void *p1, void *p2, void *p3)
{
	uint32_t inbound_drbl;
	uint8_t protocol;
	uint32_t drbl_ack;
	const struct device *dev = device_get_binding("IPC_HOST");

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	LOG_DBG("pse-host ipc-service started");

#ifdef CONFIG_HECI
	heci_init(NULL);
#endif
#ifdef CONFIG_SYS_MNG
	mng_and_boot_init(NULL);
#endif

	while (true) {
		k_sem_take(&sem_ipc_read, K_FOREVER);

		ipc_read_drbl(dev, &inbound_drbl);
		protocol =  IPC_HEADER_GET_PROTOCOL(inbound_drbl);
		__ASSERT(protocol < MAX_SERVICE_CLIENTS, "bad protocol");
		LOG_DBG("received host ipc msg, drbl=%08x",
			inbound_drbl);

		if (protocol_cb[protocol] == NULL) {
			LOG_ERR("no cb for  protocol id = %d", protocol);
			drbl_ack = inbound_drbl & (~BIT(IPC_DRBL_BUSY_OFFS));
			ipc_send_ack(dev, drbl_ack, NULL, 0);
		} else {
			protocol_cb[protocol](dev, inbound_drbl);
		}
	}
}

void host_config(void)
{
	LOG_DBG("%s", __func__);
	const struct device *dev = device_get_binding("IPC_HOST");
	struct user_app_res_table_t *res_table;

	ipc_set_rx_notify(dev, ipc_rx_handler);
	k_thread_create(&host_service_thread, host_service_stack,
			SERVICE_STACK_SIZE, ipc_rx_task, NULL, NULL, NULL,
			K_PRIO_COOP(1), 0, K_FOREVER);

	for (int i = 0; i <  CONFIG_NUM_USER_APP_AND_SERVICE; i++) {
		res_table = get_user_app_res_table(i);
		if (res_table == NULL) {
			continue;
		}
		LOG_DBG("app_handle%d:%p", i, res_table->app_handle);
		if (res_table->sys_service & HOST_SERVICE) {
			LOG_DBG("grand HOST access to App:%d", i);
			k_thread_access_grant(res_table->app_handle,
					      &dev_lock,
					      &sem_rtd3,
					      &sem_d3,
					      &rtd3_map_lock);
		}
	}
}

void host_service_init(void)
{
	k_thread_start(&host_service_thread);
}

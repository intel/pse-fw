/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <string.h>
#include <sys/sys_io.h>
#include "heci.h"
#include "sedi.h"
#include "smhi_client.h"
#include "power/reboot.h"
#include "fw_version.h"
#include <stdlib.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(smhi, CONFIG_SMHI_LOG_LEVEL);

#ifdef CONFIG_LOG_PROCESS_THREAD_SLEEP_MS
#define SMHI_HOLD_MS_BEFORE_RESET CONFIG_LOG_PROCESS_THREAD_SLEEP_MS
#else
#define SMHI_HOLD_MS_BEFORE_RESET 1000
#endif

#define HECI_CLIENT_SMHI_GUID { 0xbb579a2e,		  \
				0xcc54, 0x4450,		  \
				{ 0xb1, 0xd0, 0x5e, 0x75, \
				  0x20, 0xdc, 0xad, 0x25 } }

#define SMHI_MAX_RX_SIZE        256
#define SMHI_STACK_SIZE         1600

static heci_rx_msg_t smhi_rx_msg;
static uint8_t smhi_rx_buffer[SMHI_MAX_RX_SIZE];
static uint8_t smhi_tx_buffer[SMHI_MAX_RX_SIZE];

static K_THREAD_STACK_DEFINE(smhi_stack, SMHI_STACK_SIZE);
static struct k_thread smhi_thread;

static K_SEM_DEFINE(smhi_event_sem, 0, 1);
static uint32_t smhi_event;
static uint32_t smhi_conn_id;
static uint32_t smhi_flag;
static k_tid_t thread_using_heci[] = { &smhi_thread };

static void smhi_ret_version(smhi_msg_hdr_t *txhdr)
{
	LOG_DBG("get SMHI_GET_VERSION command");
	mrd_t m = { 0 };
	smhi_get_version_resp *resp =
		(smhi_get_version_resp *)(txhdr + 1);

	txhdr->command = SMHI_GET_VERSION;
	resp->major = (uint16_t)strtoul(FWVER_MAJOR, NULL, 0);
	resp->minor = (uint16_t)strtoul(FWVER_MINOR, NULL, 0);
	resp->hotfix = (uint16_t)strtoul(FWVER_PATCH, NULL, 0);
	resp->build = 0; /* not used by PSE */

	m.buf = (void *)txhdr;
	m.len = sizeof(smhi_get_version_resp) + sizeof(smhi_msg_hdr_t);
	heci_send(smhi_conn_id, &m);
}

static void smhi_ret_time(smhi_msg_hdr_t *txhdr)
{
	LOG_DBG("get SMHI_GET_TIME command");
	mrd_t m = { 0 };
	smhi_get_time_resp *resp =
		(smhi_get_time_resp *)(txhdr + 1);

	memset(resp, 0, sizeof(smhi_get_time_resp));
	txhdr->command = SMHI_GET_TIME;
	resp->rtc_us = sedi_rtc_get();

	m.buf = (void *)txhdr;
	m.len = sizeof(smhi_get_time_resp) + sizeof(smhi_msg_hdr_t);
	heci_send(smhi_conn_id, &m);
}

static void smhi_handle_reset_resp(smhi_msg_hdr_t *txhdr)
{
	LOG_DBG("get SMHI_PSE_RESET command");
	mrd_t m = { 0 };

	smhi_reset_resp *resp =
		(smhi_reset_resp *)(txhdr + 1);

	resp->reserved = 0;

	m.buf = (void *)txhdr;
	m.len = sizeof(smhi_reset_resp) + sizeof(smhi_msg_hdr_t);
	heci_send(smhi_conn_id, &m);
}

static void smhi_process_msg(uint8_t *buf)
{
	smhi_msg_hdr_t *rxhdr = (smhi_msg_hdr_t *)buf;
	smhi_msg_hdr_t *txhdr = (smhi_msg_hdr_t *)smhi_tx_buffer;

	txhdr->is_response = 1;
	txhdr->has_next = 0;
	txhdr->reserved = 0;
	txhdr->status = 0;

	LOG_DBG("smhi cmd =  %u", rxhdr->command);
	switch (rxhdr->command) {
	case SMHI_GET_VERSION: {
		smhi_ret_version(txhdr);
		break;
	}
	case SMHI_GET_TIME: {
		smhi_ret_time(txhdr);
		break;
	}
	case SMHI_PSE_RESET: {
		smhi_handle_reset_resp(txhdr);
		smhi_flag |= REBOOT_FLAG;
		break;
	}

	default:
		LOG_DBG("get unsupported SMHI command");
	}
}

void post_smhi_process(void)
{
	if ((smhi_flag & REBOOT_FLAG) &&
	    ((smhi_flag & SMHI_CONN_FLAG) == 0)) {
		smhi_flag &= (~REBOOT_FLAG);
		LOG_DBG("PSE rebooting, see you later");
		/* to let all logging out*/
		k_sleep(K_MSEC(SMHI_HOLD_MS_BEFORE_RESET));
		sys_reboot(0);
	}
}

static void smhi_event_callback(uint32_t event, void *arg)
{
	ARG_UNUSED(arg);
	smhi_event = event;
	k_sem_give(&smhi_event_sem);
}

static void smhi_task(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_DBG("enter");

	while (true) {
		k_sem_take(&smhi_event_sem, K_FOREVER);

		LOG_DBG("smhi new heci event %u", smhi_event);

		switch (smhi_event) {
		case HECI_EVENT_NEW_MSG:
			if (smhi_rx_msg.msg_lock != MSG_LOCKED) {
				LOG_ERR("invalid heci message");
				break;
			}

			if (smhi_rx_msg.type == HECI_CONNECT) {
				smhi_flag |= SMHI_CONN_FLAG;
				smhi_conn_id = smhi_rx_msg.connection_id;
				LOG_DBG("new conn: %u", smhi_conn_id);
			} else if (smhi_rx_msg.type == HECI_REQUEST) {
				smhi_process_msg(smhi_rx_msg.buffer);
			}

			/*
			 * send flow control after finishing one message,
			 * allow host to send new request
			 */
			heci_send_flow_control(smhi_conn_id);
			break;

		case HECI_EVENT_DISCONN:
			LOG_DBG("disconnect request conn %d",
				smhi_conn_id);
			heci_complete_disconnect(smhi_conn_id);
			smhi_flag &= (~SMHI_CONN_FLAG);
			break;

		default:
			LOG_ERR("wrong heci event %u", smhi_event);
			break;
		}
		post_smhi_process();
	}
}

static int smhi_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	int ret;
	heci_client_t smhi_client = {
		.protocol_id = HECI_CLIENT_SMHI_GUID,
		.max_msg_size = SMHI_MAX_RX_SIZE,
		.protocol_ver = 1,
		.max_n_of_connections = 1,
		.dma_header_length = 0,
		.dma_enabled = 1,
		.rx_buffer_len = SMHI_MAX_RX_SIZE,
		.event_cb = smhi_event_callback,
		.param = NULL,
		/* if not in user space it is not needed*/
		.thread_handle_list = thread_using_heci,
		.num_of_threads = sizeof(thread_using_heci) /
				  sizeof(thread_using_heci[0])
	};

	smhi_client.rx_msg = &smhi_rx_msg;
	smhi_client.rx_msg->buffer = smhi_rx_buffer;

	ret = heci_register(&smhi_client);
	if (ret) {
		LOG_ERR("failed to register smhi client %d", ret);
		return ret;
	}

	k_thread_create(&smhi_thread, smhi_stack, SMHI_STACK_SIZE,
			smhi_task, NULL, NULL, NULL,
			K_PRIO_PREEMPT(11), 0, K_NO_WAIT);
	return 0;
}

SYS_INIT(smhi_init, POST_KERNEL, 80);

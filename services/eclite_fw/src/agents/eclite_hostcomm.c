/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "eclite_hostcomm.h"
#include "eclite_dispatcher.h"
#include "platform.h"
#ifdef CONFIG_HECI
#include "heci.h"
#endif
#include "string.h"
#include "common.h"
LOG_MODULE_REGISTER(hostcomm, CONFIG_ECLITE_LOG_LEVEL);

#define EVENT_NOTIFY_CONFIG 333

#ifdef CONFIG_HECI
APP_GLOBAL_VAR(1) static heci_rx_msg_t eclite_rx_msg;
APP_GLOBAL_VAR(1) static uint32_t heci_connection_id =
	ECLITE_HECI_INVALID_CONN_ID;
APP_GLOBAL_VAR_BSS(1) static struct message_buffer eclite_rx_buffer;
APP_GLOBAL_VAR(1) k_tid_t thread_using_heci[] = { &dispatcher_task };
#endif

APP_GLOBAL_VAR_BSS(1) static struct message_buffer eclite_tx_buffer;

APP_GLOBAL_VAR(1) struct eclite_opregion_t eclite_opregion;

APP_GLOBAL_VAR(1) static struct eclite_opreg_wr_attr_tbl
	opregion_wr_chk_tbl[] = {
	/* Battery Trip Point */
	{ ECLITE_BTP1_START_OFFSET, ECLITE_BTP1_END_OFFSET,
	  ECLITE_BTP_BLKSZ },
	/* System thermal thresholds */
	{ ECLITE_SYSTH_THR_START_OFFSET, ECLITE_SYSTH_THR_END_OFFSET,
	  ECLITE_SYSTH_BLKSZ },
	/* System critical temperature */
	{ ECLITE_TH_CRIT_THR_START_OFFSET, ECLITE_TH_CRIT_THR_END_OFFSET,
	  ECLITE_SYSTH_CRIT_BLKSZ },
	/* CPU critical temperature */
	{ ECLITE_CPU_CRIT_THR_START_OFFSET, ECLITE_CPU_CRIT_THR_END_OFFSET,
	  ECLITE_CPU_CRIT_BLKSZ },
	/* DPTF Charger participant */
	{ ECLITE_CHG_PAR_START_OFFSET, ECLITE_CHG_PAR_END_OFFSET,
	  ECLITE_CHG_PAR_BLKSZ },
	/* FAN control */
	{ ECLITE_FAN_PWM_START_OFFSET, ECLITE_FAN_PWM_END_OFFSET,
	  ECLITE_FAN_PWM_BLKSZ },
	/* Enable/Disable ECLite event */
	{ ECLITE_ALERT_CTL_START_OFFSET, ECLITE_ALERT_CTL_END_OFFSET,
	  ECLITE_ALERT_CTL_BLKSZ },
	/* Enable/Disable UCSI event */
	{ ECLITE_UCSI_START_OFFSET, ECLITE_UCSI_END_OFFSET,
	  ECLITE_UCSI_BLKSZ },
};

#ifdef CONFIG_HECI
static int eclite_process_event_message(struct message_header_type
					*message_header)
{
	struct dispatcher_queue_data event_data;

	if (message_header == NULL) {
		LOG_ERR("No event message recevied");
		return ERROR;
	}

	ECLITE_LOG_DEBUG("Received event %d from Host",
			 message_header->event);

	switch (message_header->event) {
	case ECLITE_OS_EVENT_BTP_UPDATE:
		ECLITE_LOG_DEBUG("%s() Don't update OS BTP\n", __func__);
		/* Stubbing out the BTP_UPDATE as BTP Value received from OS is
		 * incorrect, need to track this issue in separate HSD with BIOS
		 * event_data.event_type = FG_EVENT;
		 * event_data.data = ECLITE_OS_EVENT_BTP_UPDATE;
		 * eclite_post_dispatcher_event(&event_data);
		 */
		break;

	case ECLITE_OS_EVENT_CRIT_BAT_UPDATE:
		event_data.event_type = FG_EVENT;
		event_data.data = ECLITE_OS_EVENT_CRIT_BAT_UPDATE;
		eclite_post_dispatcher_event(&event_data);
		break;

	case ECLITE_OS_EVENT_LOW_BAT_TH_UPDATE:
		event_data.event_type = FG_EVENT;
		event_data.data = ECLITE_OS_EVENT_LOW_BAT_TH_UPDATE;
		eclite_post_dispatcher_event(&event_data);
		break;

	case ECLITE_OS_EVENT_CRIT_TEMP_UPDATE:
		event_data.event_type = THERMAL_EVENT;
		event_data.data = ECLITE_OS_EVENT_CRIT_TEMP_UPDATE;
		eclite_post_dispatcher_event(&event_data);
		break;

	case ECLITE_OS_EVENT_THERM_ALERT_TH_UPDATE:
		event_data.event_type = THERMAL_EVENT;
		event_data.data = ECLITE_OS_EVENT_THERM_ALERT_TH_UPDATE;
		eclite_post_dispatcher_event(&event_data);
		break;

	case ECLITE_OS_EVENT_CHG1_UPDATE:
		event_data.event_type = CHG_EVENT;
		event_data.data = ECLITE_OS_EVENT_CHG1_UPDATE;
		eclite_post_dispatcher_event(&event_data);
		break;

	case ECLITE_OS_EVENT_PWM_UPDATE:
		event_data.event_type = THERMAL_EVENT;
		event_data.data = ECLITE_OS_EVENT_PWM_UPDATE;
		eclite_post_dispatcher_event(&event_data);
		break;

	case ECLITE_OS_EVENT_UCSI_UPDATE:
		event_data.event_type = UCSI_EVENT;
		event_data.data = ECLITE_OS_EVENT_UCSI_UPDATE;
		eclite_post_dispatcher_event(&event_data);
		ECLITE_LOG_DEBUG("Received UCSI event from host %d",
				 message_header->event);
		break;
	default:
		LOG_ERR("Invalid request received");
		return ERROR;
	}
	return 0;
}
#endif
static int eclite_verify_opreg_wr_access(uint16_t offset, uint16_t blocksz)
{
	uint8_t blockid = 0x0;

	for (blockid = 0x0; blockid < NUM_OF_OPR_WR_BLKS; blockid++) {
		/* Verify write allowed in the Op-Region block */
		if (offset >= opregion_wr_chk_tbl[blockid].offset_start &&
		    blocksz <= opregion_wr_chk_tbl[blockid].block_sz &&
		    (offset + blocksz - 1) <=
		    opregion_wr_chk_tbl[blockid].offset_end) {
			ECLITE_LOG_DEBUG(
				"offset_start:%d offset_end:%d block_sz:%d",
				opregion_wr_chk_tbl[blockid].offset_start,
				opregion_wr_chk_tbl[blockid].offset_end,
				opregion_wr_chk_tbl[blockid].block_sz);

			return SUCCESS;
		}
	}
	LOG_ERR("offset(%d)  blocksz(%d) write denied",
		offset, blocksz);
	return ERROR;
}

#ifdef CONFIG_HECI
static int eclite_process_data_message(struct message_buffer *buffer)
{
	uint8_t *opregion_buffer = (uint8_t *)&eclite_opregion;
	uint16_t offset;
	uint16_t length;
	struct message_header_type *message_header;
	int ret;

	message_header = (struct message_header_type *)&buffer->message_header;
	offset = message_header->offset;
	length = message_header->length;

	if ((offset + length) > MAX_OPR_LENGTH) {
		/* invalid offset and length */
		LOG_ERR("Message header length exceeding max value = %d",
			MAX_OPR_LENGTH);
		return ERROR;
	}

	if (message_header->read_write == ECLITE_HEADER_OPR_READ) {
		struct message_buffer *send_buf = &eclite_tx_buffer;
		struct message_header_type *message_header2;
		mrd_t mrd_message = { 0 };

		message_header2 = (struct message_header_type *)
				  &send_buf->message_header;
		message_header2->revision = ECLITE_OPR_REVISION;
		message_header2->data_type = ECLITE_HEADER_TYPE_DATA;
		message_header2->read_write = ECLITE_HEADER_OPR_READ;
		message_header2->offset = offset;
		message_header2->length = length;
		ECLITE_LOG_DEBUG(
			"ECLITE_HEADER_OPR_READ offset: %d, length: %d",
			offset,
			length);

		memcpy(send_buf->data, opregion_buffer + offset, length);
		mrd_message.buf = send_buf;
		mrd_message.len = sizeof(struct message_header_type)
				  + length;
		ret = heci_send(heci_connection_id, &mrd_message);
		ECLITE_LOG_DEBUG("Heci send ret val %d", ret);
	} else if (message_header->read_write == ECLITE_HEADER_OPR_WRITE) {
		uint8_t *receive_buffer = buffer->data;
		int ret;

		ECLITE_LOG_DEBUG(
			"ECLITE_HEADER_OPR_WRITE offset:%d, length: %d",
			offset,
			length);

		ret = eclite_verify_opreg_wr_access(offset, length);
		if (ret == SUCCESS) {
			memcpy(opregion_buffer + offset, receive_buffer,
			       length);
		} else {
			/* no write permission */
			LOG_ERR("Opregion write access failed");
			return ERROR;
		}
	} else {
		/* invalid request */
		LOG_ERR("Invalid heci request");
		return ERROR;
	}
	return 0;
}
#endif

#ifdef CONFIG_HECI
static int eclite_process_msg(struct message_buffer *buffer)
{
	struct message_header_type *message_header = NULL;
	int ret = ERROR;

	if (buffer == NULL) {
		LOG_ERR("HECI buffer null");
		return ERROR;
	}

	message_header = (struct message_header_type *)&buffer->message_header;
	if (message_header->data_type == ECLITE_HEADER_TYPE_DATA) {
		ret = eclite_process_data_message(buffer);
		ECLITE_LOG_DEBUG("Process data msg ret: %d", ret);
	}

	if (message_header->event) {
		ret = eclite_process_event_message(message_header);
		ECLITE_LOG_DEBUG("Process event msg ret: %d", ret);
	}

	return ret;
}
#endif

int eclite_send_event(uint32_t event)
{
	struct message_buffer *buffer = &eclite_tx_buffer;
	struct message_header_type *message_header =
		(struct message_header_type *)&buffer->message_header;

	/* update the driver via IPC */
	message_header->revision = ECLITE_OPR_REVISION;
	message_header->event      = event;
	message_header->data_type = ECLITE_HEADER_TYPE_EVENT;
#ifdef CONFIG_HECI
	int ret;

	mrd_t mrd_message  = { 0 };

	mrd_message.buf = buffer;
	mrd_message.len = sizeof(struct message_header_type);

	if (heci_connection_id == ECLITE_HECI_INVALID_CONN_ID) {
		LOG_WRN("Invalid HECI connection ID");
		return ERROR;
	}

	if (!eclite_opregion.event_notify_config) {
		return 0;
	}

	ECLITE_LOG_DEBUG("Sending Event %u to OS", event);

	ret = heci_send(heci_connection_id, &mrd_message);
	if (ret) {
		/* ret = heci_send_flow_control(heci_connection_id); */
		LOG_WRN("HECI send failure");
		return ret;
	}
#endif
	return 0;
}

#ifdef CONFIG_HECI
void check_events_config_request(uint32_t event)
{
	struct message_header_type *message_header = NULL;
	struct message_buffer *buffer;

	if (event != HECI_EVENT_NEW_MSG) {
		return;
	}

	if (eclite_rx_msg.type != HECI_REQUEST) {
		return;
	}

	buffer = (struct message_buffer *) eclite_rx_msg.buffer;

	if (buffer == NULL) {
		return;
	}

	message_header = (struct message_header_type *)
			 &buffer->message_header;

	if (message_header->data_type != ECLITE_HEADER_TYPE_DATA) {
		return;
	}

	if (message_header->read_write != ECLITE_HEADER_OPR_WRITE) {
		return;
	}

	if (message_header->offset != EVENT_NOTIFY_CONFIG) {
		return;
	}

	eclite_opregion.event_notify_config = buffer->data[0];
	printk("EC Events: %sabled\n", (eclite_opregion.event_notify_config) ?
	       "En" : "Dis");
}
#endif

#ifdef CONFIG_HECI
static void eclite_event_callback(uint32_t event, void *param)
{
	ARG_UNUSED(param);

	struct dispatcher_queue_data heci_data;

	check_events_config_request(event);

	heci_data.event_type = HECI_EVENT;
	heci_data.data = event;

	ECLITE_LOG_DEBUG("HECI Event %u", event);
	eclite_post_dispatcher_event(&heci_data);
}
#endif

int eclite_heci_event_process(uint32_t event)
{
#ifdef CONFIG_HECI
	int ret = ERROR;
	struct dispatcher_queue_data event_data;

	switch (event) {
	case HECI_EVENT_NEW_MSG:
		if (eclite_rx_msg.msg_lock != MSG_LOCKED) {
			LOG_ERR("Invalid heci message");
			ret = ERROR;
			break;
		}
		if (eclite_rx_msg.type == HECI_CONNECT) {
			heci_connection_id = eclite_rx_msg.connection_id;
			ECLITE_LOG_DEBUG("New conn: %u", heci_connection_id);
			if (cpu_thermal_enable) {
				k_timer_start(&dispatcher_timer, K_NO_WAIT,
				    K_MSEC(CONFIG_ECLITE_POLLING_TIMER_PERIOD));
			}
		} else if (eclite_rx_msg.type == HECI_REQUEST) {
			ECLITE_LOG_DEBUG(
				"Eclite process msg HECI Request");
			ret = eclite_process_msg((struct message_buffer *)
						 eclite_rx_msg.buffer);
			ECLITE_LOG_DEBUG("Eclite process msg sts ret:%d", ret);
		}
		/*
		 * send flow control after finishing one message,
		 * allow host to send new request
		 */
		if (heci_connection_id != ECLITE_HECI_INVALID_CONN_ID) {
			if (heci_send_flow_control(heci_connection_id)) {
				ret = SUCCESS;
			} else {
				LOG_ERR("HECI send flow control failure");
				ret = ERROR;
			}
		}
		break;
	case HECI_EVENT_DISCONN:
		ECLITE_LOG_DEBUG("Disconnect conn %u", heci_connection_id);
		if (heci_complete_disconnect(heci_connection_id) == 0) {
			heci_connection_id = ECLITE_HECI_INVALID_CONN_ID;
			if (cpu_thermal_enable) {
				k_timer_stop(&dispatcher_timer);
				k_msgq_purge(&dispatcher_queue);
			}

			/* Turn off the FAN */
			eclite_opregion.pwm_dutycyle = 0;
			event_data.event_type = THERMAL_EVENT;
			event_data.data = ECLITE_OS_EVENT_PWM_UPDATE;
			eclite_post_dispatcher_event(&event_data);

			ret = SUCCESS;
		} else {
			LOG_ERR("HECI disconnect failure");
			ret = ERROR;
		}
		break;
	default:
		LOG_ERR("Wrong heci event %u", event);
		ret = ERROR;
		break;
	}
	return ret;
#else
	return 0;
#endif
}

int init_host_communication(void)
{
#ifdef CONFIG_HECI
	int ret;
	heci_client_t heci_client = {
		.protocol_id = HECI_CLIENT_ECLITE_GUID,
		.max_msg_size = ECLITE_HECI_MAX_RX_SIZE,
		.protocol_ver = ECLITE_HECI_PROTOCOL_VER,
		.max_n_of_connections = ECLITE_HECI_MAX_CONNECTIONS,
		.dma_header_length = 0,
		.dma_enabled = 0,
		.rx_buffer_len = ECLITE_HECI_MAX_RX_SIZE,
		.event_cb = eclite_event_callback,
		.thread_handle_list = thread_using_heci,
		.num_of_threads = sizeof(thread_using_heci) /
				  sizeof(thread_using_heci[0])
	};

	ECLITE_LOG_DEBUG("Start HECI");

	/* prepare the heci client parameters */
	/* it is os 1.0v for pse */
	heci_client.rx_msg = &eclite_rx_msg;
	heci_client.rx_msg->buffer = (uint8_t *)&eclite_rx_buffer;

	/* Register to HECI driver */
	ret = heci_register(&heci_client);

	if (ret) {
		LOG_ERR("HECI registration error %d", ret);
		return ret;
	}
	return 0;
#else
	return 0;
#endif
}

/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file thingsboard.c
 *
 * \brief This file contains implementations for thingsboard specific calls
 *
 * \see thingsboard.h and adapter.h
 *
 */

#include <stdio.h>
#include "thingsboard.h"
#include <common/credentials.h>
#include <common/pse_app_framework.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(OOB_THINGSBOARD);

#define MAX_MSG_BUF_LEN 64
VAR_DEFINER_BSS char pub_buf[MAX_MSG_BUF_LEN];
VAR_DEFINER_BSS char tb_payload[MAX_MSG_BUF_LEN];

extern VAR_DEFINER struct cloud_credentials *creds;

VAR_DEFINER char *thingsboard_mqtt_subs_topics[NUM_THINGSBOARD_SUB_TOPICS] = {
	THINGSBOARD_RPC_SUB_TOPIC
};

/** Function which returns thingsboard publish topic for mqtt */
char *thingsboard_get_mqtt_pub_topic(enum app_message_type pub_msg_type)
{
	size_t msg_buf_size = MAX_MSG_BUF_LEN;

	if (pub_msg_type == STATIC) {
		return THINGSBOARD_MQTT_PUB_ATTRIB_TOPIC;
	} else if (pub_msg_type == API) {
		thingsboard_prepare_pub_ack_topic(tb_payload,
						  pub_buf, msg_buf_size);
		return pub_buf;
	} else if ((pub_msg_type == DYNAMIC) || (pub_msg_type == EVENT)) {
		return THINGSBOARD_MQTT_PUB_TELEM_TOPIC;
	} else {
		return NULL;
	}
}

/** Function which returns the no of thingsboard subscription_topics */
int thingsboard_get_mqtt_sub_topics_count(void)
{
	return NUM_THINGSBOARD_SUB_TOPICS;
}

int thingsboard_get_mqtt_sub_topics(
	char sub_topics[NUM_THINGSBOARD_SUB_TOPICS][MAX_MQTT_SUBS_TOPIC_LEN])
{
	int ret = OOB_SUCCESS;
	int sub_topic_len = 0;

	for (int i = 0; i < NUM_THINGSBOARD_SUB_TOPICS; i++) {
		sub_topic_len = strlen(thingsboard_mqtt_subs_topics[i]);
		if (sub_topic_len < sizeof(sub_topics[i])) {
			memcpy(sub_topics[i],
			       thingsboard_mqtt_subs_topics[i],
			       sub_topic_len);
		} else {
			ret = OOB_ERR_ADAPTER_SUB_TOPIC_CPY;
		}
	}
	return ret;
}

void thingsboard_prepare_attrib_pub_message(char *msg_buf,
					    size_t msg_buf_size,
					    const char *key,
					    const char *value)
{
	snprintf(msg_buf,
		 msg_buf_size,
		 THINGSBOARD_ATTRIBUTE_MESSAGE_TEMPLATE,
		 key, value);
}

void thingsboard_prepare_pub_ack_topic(char *payload,
				       char *msg_buf, size_t msg_buf_size)
{
	char *temp_buf = NULL;

	memset(msg_buf, '\0', msg_buf_size);

	/* Extract the request ID from payload */
	temp_buf = strstr(payload, THINGSBOARD_REQUEST_ID);
	if (temp_buf != NULL) {
		snprintf(msg_buf, msg_buf_size, THINGSBOARD_RPC_REPLY_TOPIC, temp_buf);
	}
}

enum oob_messages thingsboard_process_message(uint8_t *payload,
					      uint32_t payload_length,
					      uint8_t *topic,
					      uint32_t topic_length,
					      char *next_msg,
					      size_t next_msg_size)
{
	char *oob_cmd = NULL;
	enum oob_messages oob_cmd_msg = IGNORE;

	if (topic_length > MAX_MSG_BUF_LEN) {
		LOG_ERR("Insufficient topic length");
		return IGNORE;
	}

	memset(tb_payload, 0x00, MAX_MSG_BUF_LEN);
	memcpy(tb_payload, topic, topic_length);

	memset(next_msg, 0x00, next_msg_size);

	if (!strstr(topic, THINGSBOARD_RPC_REPLY_TOPIC)) {
		oob_cmd = strstr(payload, THINGSBOARD_REBOOT_PARAMS_METHOD);
		if (oob_cmd != NULL) {
			oob_cmd_msg = REBOOT;
			goto oob_cmd_exit;
		}

		oob_cmd = strstr(payload, THINGSBOARD_POWERUP_PARAMS_METHOD);
		if (oob_cmd != NULL) {
			oob_cmd_msg = POWERON;
			goto oob_cmd_exit;
		}

		oob_cmd = strstr(payload, THINGSBOARD_POWERDOWN_PARAMS_METHOD);
		if (oob_cmd != NULL) {
			oob_cmd_msg = POWEROFF;
			goto oob_cmd_exit;
		}

		oob_cmd = strstr(payload, THINGSBOARD_DECOMMISSION_PARAMS_METHOD);
		if (oob_cmd != NULL) {
			oob_cmd_msg = DECOMMISSION;
			goto oob_cmd_exit;
		}
	}

oob_cmd_exit:

	if (oob_cmd != NULL) {
		snprintf(next_msg, next_msg_size,
			 THINGSBOARD_ATTRIBUTE_MESSAGE_TEMPLATE,
			 "status", "200");
	}

	return oob_cmd_msg;
}

void thingsboard_prepare_log_message(char *msg_buf, size_t msg_buf_size,
				     const char *message)
{
	snprintf(msg_buf,
		 msg_buf_size,
		 THINGSBOARD_EVENT_MESSAGE_TEMPLATE,
		 message);
}

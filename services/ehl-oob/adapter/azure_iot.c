/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file azure_iot.c
 *
 * \brief This file contains implementations for azure_iot specific calls
 *
 * \see azure_iot.h and adapter.h
 *
 */

#include <stdio.h>
#include "azure_iot.h"
#include <common/credentials.h>
#include <common/pse_app_framework.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(OOB_AZURE_IOT, CONFIG_OOB_LOGGING);

#define MAX_MSG_BUF_LEN 64
VAR_DEFINER_BSS char msg_buffer[MAX_MSG_BUF_LEN];
VAR_DEFINER_BSS char az_payload[MAX_MSG_BUF_LEN];

VAR_DEFINER char *azure_iot_mqtt_subs_topics[NUM_AZURE_IOT_SUB_TOPICS] = {
	AZURE_IOT_RPC_SUB_TOPIC
};

/** Function which returns Azure IOT publish topic for mqtt */
char *azure_iot_get_mqtt_pub_topic(enum app_message_type pub_msg_type)
{
	size_t msg_buf_size = MAX_MSG_BUF_LEN;

	if (pub_msg_type == STATIC) {
		return AZURE_IOT_MQTT_PUB_ATTRIB_TOPIC;
	} else if (pub_msg_type == API) {
		azure_iot_prepare_pub_ack_topic(az_payload,
						msg_buffer, msg_buf_size);
		return msg_buffer;
	} else if ((pub_msg_type == DYNAMIC) || (pub_msg_type == EVENT)) {
		azure_iot_prepare_pub_telem_topic(msg_buffer,
						  msg_buf_size,
						  creds->mqtt_client_id);
		return msg_buffer;
	} else {
		return NULL;
	}
}

/** Function which returns the no of azure_iot subscription_topics */
int azure_iot_get_mqtt_sub_topics_count(void)
{
	return NUM_AZURE_IOT_SUB_TOPICS;
}

int azure_iot_get_mqtt_sub_topics(
	char sub_topics[NUM_AZURE_IOT_SUB_TOPICS][MAX_MQTT_SUBS_TOPIC_LEN])
{
	int flag = OOB_SUCCESS;

	for (int i = 0; i < NUM_AZURE_IOT_SUB_TOPICS; i++) {
		if (strlen(azure_iot_mqtt_subs_topics[i]) < sizeof(sub_topics[i])) {
			memcpy(sub_topics[i],
			       azure_iot_mqtt_subs_topics[i],
			       strlen(azure_iot_mqtt_subs_topics[i]));
		} else {
			flag = OOB_ERR_ADAPTER_SUB_TOPIC_CPY;
		}
	}
	return flag;
}

void azure_iot_prepare_attrib_pub_message(char *msg_buf,
					  size_t msg_buf_size,
					  const char *key,
					  const char *value)
{
	snprintf(msg_buf,
		 msg_buf_size,
		 AZURE_IOT_ATTRIBUTE_MESSAGE_TEMPLATE,
		 key, value);
}

void azure_iot_prepare_pub_telem_topic(char *msg_buf,
				       size_t msg_buf_size,
				       const char *value)
{
	snprintf(msg_buf,
		 msg_buf_size,
		 AZURE_IOT_MQTT_PUB_TELEM_TOPIC,
		 value);
}

void azure_iot_prepare_pub_ack_topic(char *payload,
				     char *msg_buf, size_t msg_buf_size)
{
	char *temp_buf = NULL;

	memset(msg_buf, '\0', msg_buf_size);

	/* Extract the request ID from payload */
	temp_buf = strstr(payload, AZURE_IOT_REQUEST_ID);
	if (temp_buf != NULL) {
		snprintf(msg_buf, msg_buf_size, AZURE_IOT_RPC_REPLY_TOPIC, temp_buf);
	}
}

enum oob_messages azure_iot_process_message(uint8_t *payload,
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

	memset(az_payload, 0x00, MAX_MSG_BUF_LEN);
	memcpy(az_payload, topic, topic_length);

	memset(next_msg, 0x00, next_msg_size);

	if (!strstr(topic, AZURE_IOT_RPC_REPLY_TOPIC)) {
		LOG_INF("Processing message azure_iot");
		oob_cmd = strstr(topic, AZURE_IOT_REBOOT_PARAMS_METHOD);
		if (oob_cmd != NULL) {
			oob_cmd_msg = REBOOT;
			goto oob_cmd_exit;
		}

		oob_cmd = strstr(topic, AZURE_IOT_POWERUP_PARAMS_METHOD);
		if (oob_cmd != NULL) {
			oob_cmd_msg = POWERON;
			goto oob_cmd_exit;
		}

		oob_cmd = strstr(topic, AZURE_IOT_POWERDOWN_PARAMS_METHOD);
		if (oob_cmd != NULL) {
			oob_cmd_msg = POWEROFF;
			goto oob_cmd_exit;
		}

		oob_cmd = strstr(topic, AZURE_IOT_DECOMMISSION_PARAMS_METHOD);
		if (oob_cmd != NULL) {
			oob_cmd_msg = DECOMMISSION;
			goto oob_cmd_exit;
		}
	}

oob_cmd_exit:

	if (oob_cmd != NULL) {
		snprintf(next_msg, next_msg_size,
			 AZURE_IOT_ATTRIBUTE_MESSAGE_TEMPLATE,
			 "status", "200");
	}

	return oob_cmd_msg;
}

void azure_iot_prepare_log_message(char *msg_buf, size_t msg_buf_size,
				   const char *message)
{
	snprintf(msg_buf,
		 msg_buf_size,
		 AZURE_IOT_EVENT_MESSAGE_TEMPLATE,
		 AZURE_IOT_EVENT_GENERIC,
		 message);
}


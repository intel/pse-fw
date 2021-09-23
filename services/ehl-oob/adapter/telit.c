/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file telit.c
 *
 * \brief This file contains implementations for telit specific calls
 *
 * \see telit.h and adapter.h
 *
 */

#include <stdio.h>
#include "telit.h"
#include <common/credentials.h>
#include <common/pse_app_framework.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(OOB_TELIT);

extern VAR_DEFINER struct cloud_credentials *creds;

VAR_DEFINER char telit_ignore_list[TELIT_NUM_IGNORE_MESSAGES]
[TELIT_MAX_IGNORE_MESSAGE_LENGTH] = {
	TELIT_CMD_REPLY_MESSAGE
};

VAR_DEFINER char *telit_mqtt_subs_topics[NUM_TELIT_SUB_TOPICS] = {
	TELIT_REPLY_SUB_TOPIC,
	TELIT_REPLYZ_SUB_TOPIC,
	TELIT_NOTIFY_SUB_TOPIC
};

/** Function which returns telit publish topic for mqtt */
char *telit_get_mqtt_pub_topic(enum app_message_type pub_msg_type)
{
	ARG_UNUSED(pub_msg_type);
	return TELIT_MQTT_PUB_TOPIC;
}

/** Function which returns the no of telit subscription_topics */
int telit_get_mqtt_sub_topics_count(void)
{
	return NUM_TELIT_SUB_TOPICS;
}

int telit_get_mqtt_sub_topics(
	char sub_topics[NUM_TELIT_SUB_TOPICS][MAX_MQTT_SUBS_TOPIC_LEN])
{
	int ret = OOB_SUCCESS;
	int sub_topic_len;

	for (int i = 0; i < NUM_TELIT_SUB_TOPICS; i++) {
		sub_topic_len = strlen(telit_mqtt_subs_topics[i]);
		if (sub_topic_len < sizeof(sub_topics[i])) {
			memcpy(sub_topics[i],
			       telit_mqtt_subs_topics[i],
			       sub_topic_len);
		} else {
			ret = OOB_ERR_ADAPTER_SUB_TOPIC_CPY;
		}
	}
	return ret;
}

void telit_prepare_attrib_pub_message(char *msg_buf,
				      size_t msg_buf_size,
				      const char *key,
				      const char *value)
{
	snprintf(msg_buf,
		 msg_buf_size,
		 TELIT_ATTRIBUTE_MESSAGE_TEMPLATE,
		 creds->username, key, value);
}

/**
 * Internal TELIT specific function to iterate over a pre-defined list of
 * messages and check if it needs to be ignored. telit_process_message
 * calls this one
 *
 * @param [in] msg char *
 * @param [in] msg_len int
 * @retval bool true if to be ignored
 * @retval bool false if not to be ignored
 **/
bool telit_check_exists_ignore_list(char *msg, int msg_len)
{
	for (int i = 0; i < ARRAY_SIZE(telit_ignore_list); i++) {
		if (!strncmp(msg, telit_ignore_list[i], msg_len)) {
			return true;
		}
	}
	return false;
}

/**
 * Internal TELIT specific function to prepare a mailbox_check
 * message. Called by telit_process_message in response to a
 * notify/mailbox activity message. This message is classified
 * as a RELAY as it is an intermediate message that needs to be
 * exchanged to get to oob_cmd: REBOOT, DECOMMIISION etc.
 *
 * \see common/utils for all messages types.
 *
 * @param [in][out] msg_buf char *
 * @param [in] msg_buf_size size_t
 * @retval void
 **/
void telit_prepare_mailbox_check(char *msg_buf, size_t msg_buf_size)
{
	snprintf(msg_buf,
		 msg_buf_size,
		 TELIT_MAILBOX_CHECK_MESSAGE_TEMPLATE,
		 1);
}

enum oob_messages telit_process_message(uint8_t *payload,
					uint32_t payload_length,
					uint8_t *topic,
					uint32_t topic_length,
					char *next_msg,
					size_t next_msg_size)
{
	char *method_exec = NULL;
	char *oob_cmd = NULL;
	char *thing_verify = NULL;

	if (!strncmp(topic,
		     TELIT_REPLY_TOPIC,
		     topic_length) &&
	    telit_check_exists_ignore_list(payload,
					   payload_length)) {
		next_msg = NULL;
		return IGNORE;
	}

	if (!strncmp(topic,
		     TELIT_NOTIFY_TOPIC,
		     strlen(TELIT_NOTIFY_TOPIC))) {
		telit_prepare_mailbox_check(next_msg,
					    next_msg_size);
		return RELAY;
	}

	if (!strncmp(topic,
		     TELIT_REPLY_TOPIC,
		     strlen(TELIT_REPLY_TOPIC))) {
		method_exec = strstr(payload, TELIT_METHOD_EXEC);
		thing_verify = strstr(payload, creds->username);

		oob_cmd = strstr(payload, TELIT_REBOOT_PARAMS_METHOD);
		if (method_exec != NULL &&
		    oob_cmd != NULL &&
		    thing_verify != NULL) {
			telit_prepare_mailbox_ack(payload, next_msg, next_msg_size);
			return REBOOT;
		}

		oob_cmd = strstr(payload,
				 TELIT_POWERUP_PARAMS_METHOD);
		if (method_exec != NULL &&
		    oob_cmd != NULL &&
		    thing_verify != NULL) {
			telit_prepare_mailbox_ack(payload, next_msg, next_msg_size);
			return POWERON;
		}

		oob_cmd = strstr(payload,
				 TELIT_POWERDOWN_PARAMS_METHOD);
		if (method_exec != NULL &&
		    oob_cmd != NULL &&
		    thing_verify != NULL) {
			telit_prepare_mailbox_ack(payload, next_msg, next_msg_size);
			return POWEROFF;
		}

		oob_cmd = strstr(payload,
				 TELIT_DECOMMISSION_PARAMS_METHOD);
		if (method_exec != NULL &&
		    oob_cmd != NULL &&
		    thing_verify != NULL) {
			telit_prepare_mailbox_ack(payload, next_msg, next_msg_size);
			return DECOMMISSION;
		}
	}
	return IGNORE;
}

void telit_prepare_log_message(char *msg_buf, size_t msg_buf_size,
			       const char *message)
{
	snprintf(msg_buf,
		 msg_buf_size,
		 TELIT_LOG_MESSAGE_TEMPLATE,
		 creds->username,
		 message);
}

void telit_prepare_mailbox_ack(char *payload, char *msg_buf,
			       size_t msg_buf_size)
{
	char *temp_buf, bufout[TELIT_ID_LEN];
	int i = 2;

	memset(msg_buf, '\0', msg_buf_size);

	/* Extract the ID from payload */
	temp_buf = strstr(payload, TELIT_REQUEST_ID);
	if (temp_buf == NULL) {
		return;
	}

	while (i--) {
		temp_buf = strchr(temp_buf, '\"');
		temp_buf++;
	}

	memcpy(bufout, temp_buf, TELIT_ID_LEN);

	snprintf(msg_buf, msg_buf_size,
		 TELIT_MAILBOX_ACK_MESSAGE_TEMPLATE,
		 bufout, TELIT_MAILBOX_ACK_RESPONSE);
}


/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file telit.h
 *
 * \brief This file contains declarations for TELIT specific calls which get
 * are used in adapter.c
 *
 */

#ifndef _TELIT_H_
#define _TELIT_H_

#include <zephyr.h>
#include <common/pse_app_framework.h>
#include <common/utils.h>
#include <mqtt_client/mqtt_client.h>

/** Used during verification of commn_name from the certificates
 * during SSL handshake
 * This constant is used as an argument to MbedTls x509 verify function
 * to validate the certificate
 */
#if defined(CONFIG_MQTT_LIB_TLS)
#define TELIT_TLS_SNI_HOSTNAME "*.devicewise.com"
#endif

/** No of telit subscription topics */
#define NUM_TELIT_SUB_TOPICS 3

/** TELIT specific messages keys known as params */
#define TELIT_REBOOT_PARAMS_METHOD "reboot_device"
#define TELIT_DECOMMISSION_PARAMS_METHOD "decommision_device"
#define TELIT_POWERDOWN_PARAMS_METHOD "powerdown_device"
#define TELIT_POWERUP_PARAMS_METHOD "powerup_device"
#define TELIT_METHOD_EXEC "method.exec"

/** TELIT specific messages */
#define TELIT_REQUEST_ID "id"
#define TELIT_ID_LEN 24
#define TELIT_MAILBOX_ACK_RESPONSE "Command Received"

#define TELIT_MAILBOX_ACK_MESSAGE_TEMPLATE	     \
	"{\"cmd\":{\"command\":\"mailbox.ack\","     \
	"\"params\":{\"id\":\"%s\",\"errorCode\":0," \
	"\"params\":{\"Response\":\"%s\"}}}}"

#define TELIT_LOG_MESSAGE_TEMPLATE		 \
	"{\"cmd\":{\"command\":\"log.publish\"," \
	"\"params\":{\"thingKey\":\"%s\",\"msg\":\"%s\"}}}"

#define TELIT_MAILBOX_CHECK_MESSAGE_TEMPLATE	   \
	"{\"cmd\":{\"command\":\"mailbox.check\"," \
	"\"params\":{\"autocomplete\": false,\"limit\":%d}}}"

#define TELIT_ATTRIBUTE_MESSAGE_TEMPLATE	       \
	"{\"cmd\":{\"command\":\"attribute.publish\"," \
	"\"params\":{\"thingKey\":\"%s\",\"key\":\"%s\",\"value\":\"%s\"}}}"

/** TELIT publish topic.. All outgoing messages are published on this topic */
#define TELIT_MQTT_PUB_TOPIC "api"

/** TELIT topic on which server post most replies */
#define TELIT_REPLY_TOPIC "reply"
/** TELIT topic on which server posts messages for every trigger. This is
 * topic on which a mailbox message is received.
 */
#define TELIT_NOTIFY_TOPIC "notify/mailbox_activity"

#define TELIT_NUM_IGNORE_MESSAGES 1
#define TELIT_MAX_IGNORE_MESSAGE_LENGTH 100

/** TELIT message no1 to be ignored */
#define TELIT_CMD_REPLY_MESSAGE "{\"cmd\":{\"success\":true}}"

/** TELIT subscription topics */
#define TELIT_REPLY_SUB_TOPIC "reply/#"
#define TELIT_REPLYZ_SUB_TOPIC "replyz/#"
#define TELIT_NOTIFY_SUB_TOPIC "notify/#"

/** All the messages sent by the server doesn't have to processed and
 * can be IGNORED if the adapter chooses.
 *
 * \note Every adapter should override the ignore_list according to
 * its own needs
 */
extern VAR_DEFINER char ignore_list[TELIT_NUM_IGNORE_MESSAGES]
[TELIT_MAX_IGNORE_MESSAGE_LENGTH];

/**
 * TELIT specific method to get mqtt subscription topics count
 *
 * @retval int count of topics
 **/
int telit_get_mqtt_sub_topics_count(void);

/**
 * TELIT specific method to get mqtt subscription topics
 *
 * @param [in][out] char sub_topics
 * [NUM_TELIT_SUB_TOPICS][MAX_MQTT_SUBS_TOPIC_LEN]
 * @retval OOB_ERR_ADAPTER_SUB_TOPIC_CPY on failure , OOB_SUCCESS otherwise
 **/
int telit_get_mqtt_sub_topics(
	char sub_topics[NUM_TELIT_SUB_TOPICS][MAX_MQTT_SUBS_TOPIC_LEN]);

/**
 * TELIT specific method to get mqtt publish topic
 *
 * @param [in] enum app_message_type
 * @retval char *
 **/
char *telit_get_mqtt_pub_topic(enum app_message_type pub_msg_type);

/**
 * TELIT specific call to create an attribute message
 * for static telemetry publish request. Static telemetry
 * is called an attribute in TELIT
 *
 * @param [in][out] msg_buf char *
 * @param [in]      msg_buf_size size_t
 * @param [in]      key const char * Key for the attribute
 * @param [in]      key const value * value for that key
 * @retval void
 **/
void telit_prepare_attrib_pub_message(char *msg_buf,
				      size_t msg_buf_size,
				      const char *key,
				      const char *value);

/**
 * TELIT specific call to create a event message depending
 * on the parameters passed. An event message is
 * anything that needs to notified to the server. Event is sometimes
 * called a log event in TELIT
 *
 * @param [in][out] msg_buf char *
 * @param [in]      msg_buf size_t
 * @param [in]      message const char *
 * @retval void
 **/
void telit_prepare_log_message(char *msg_buf, size_t msg_buf_size,
			       const char *message);

/**
 * TELIT specific call to send acknowledgment on successful
 * completion of method execution.
 *
 * @param [in]      payload char *
 * @param [in][out] msg_buf char *
 * @param [in]      msg_buf_size size_t
 * @retval void
 **/
void telit_prepare_mailbox_ack(char *payload, char *msg_buf,
			       size_t msg_buf_size);

/**
 * @brief Helper function for mqtt_evt_handler when message of
 * type MQTT_EVT_PUBLISH is sent
 *
 * @param[in] payload: payload received
 * @param[in] payload_size: size of the payload
 * @param[in] topic: topic on which message was received
 * @param[in] topic_len: length of the topic
 *
 * @note: This function runs in interrupt context for mqtt. Make
 * sure it is not performing heavy tasks.
 */
enum oob_messages telit_process_message(uint8_t *payload,
					uint32_t payload_length,
					uint8_t *topic,
					uint32_t topic_length,
					char *next_msg,
					size_t next_msg_size);



#endif

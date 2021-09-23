/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file thingsboard.h
 *
 * \brief This file contains declarations for THINGSBOARD specific calls which get
 * are used in adapter.c
 *
 */

#ifndef _THINGSBOARD_H_
#define _THINGSBOARD_H_

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
#define THINGSBOARD_TLS_SNI_HOSTNAME "portal.thingsboard.com"
#endif

/** No of thingsboard subscription topics */
#define NUM_THINGSBOARD_SUB_TOPICS 1

/** THINGSBOARD specific messages keys known as params */
#define THINGSBOARD_REBOOT_PARAMS_METHOD "reboot_device"
#define THINGSBOARD_DECOMMISSION_PARAMS_METHOD "decommission_device"
#define THINGSBOARD_POWERDOWN_PARAMS_METHOD "powerdown_device"
#define THINGSBOARD_POWERUP_PARAMS_METHOD "powerup_device"

/** THINGSBOARD specific messages */
#define THINGSBOARD_REQUEST_ID "requestID"

/** Telemetry message format: {"key":"value"} */
#define THINGSBOARD_TELEMETRY_MESSAGE_TEMPLATE \
	"{\"%s\":\"%s\"}"

/** Attribute message format: {"key":"value"} */
#define THINGSBOARD_ATTRIBUTE_MESSAGE_TEMPLATE \
	"{\"%s\":\"%s\"}"

/** Event message format: {"message"} */
#define THINGSBOARD_EVENT_MESSAGE_TEMPLATE \
	"{\"event\":\"%s\"}"


/** THINGSBOARD publish topic.. All outgoing messages are published on this topic */
#define THINGSBOARD_MQTT_PUB_ATTRIB_TOPIC "v1/devices/me/attributes"
#define THINGSBOARD_MQTT_PUB_TELEM_TOPIC "v1/devices/me/telemetry"

/** THINGSBOARD topic on which server post most replies */
#define THINGSBOARD_RPC_REPLY_TOPIC "v1/devices/me/rpc/response/%s"

/** THINGSBOARD topic on which server posts messages for every trigger */
#define THINGSBOARD_RPC_SUB_TOPIC "v1/devices/me/rpc/request/+"

/**
 * THINGSBOARD specific method to get mqtt subscription topics count
 *
 * @retval int count of topics
 **/
int thingsboard_get_mqtt_sub_topics_count(void);

/**
 * THINGSBOARD specific method to get mqtt subscription topics
 *
 * @param [in][out] char sub_topics
 * [NUM_THINGSBOARD_SUB_TOPICS][MAX_MQTT_SUBS_TOPIC_LEN]
 * @retval OOB_ERR_ADAPTER_SUB_TOPIC_CPY on failure , OOB_SUCCESS otherwise
 **/
int thingsboard_get_mqtt_sub_topics(
	char sub_topics[NUM_THINGSBOARD_SUB_TOPICS][MAX_MQTT_SUBS_TOPIC_LEN]);

/**
 * THINGSBOARD specific method to get mqtt publish topic
 *
 * @param [in] enum of app_message_type
 * @retval char *
 **/
char *thingsboard_get_mqtt_pub_topic(enum app_message_type pub_msg_type);

/**
 * THINGSBOARD specific call to create an attribute message
 * for static telemetry publish request. Static telemetry
 * is called an attribute in THINGSBOARD
 *
 * @param [in][out] msg_buf char *
 * @param [in]      msg_buf_size size_t
 * @param [in]      key const char * Key for the attribute
 * @param [in]      key const value * value for that key
 * @retval void
 **/
void thingsboard_prepare_attrib_pub_message(char *msg_buf,
					    size_t msg_buf_size,
					    const char *key,
					    const char *value);

/**
 * THINGSBOARD specific call to create a event message depending
 * on the parameters passed. An event message is
 * anything that needs to notified to the server. Event is sometimes
 * called a log event in THINGSBOARD
 *
 * @param [in][out] msg_buf char *
 * @param [in]      msg_buf size_t
 * @param [in]      message const char *
 * @retval void
 **/
void thingsboard_prepare_log_message(char *msg_buf, size_t msg_buf_size,
				     const char *message);


/**
 * THINGSBOARD specific call to send acknowledgment on successful
 * completion of method execution.
 *
 * @param [in]      payload: payload received
 * @param [in][out] msg_buf: topic message to be passed
 * @param [in]      msg_buf_size: size of the topic
 * @retval void
 **/
void thingsboard_prepare_pub_ack_topic(char *payload, char *msg_buf,
				       size_t msg_buf_size);

/**
 * @brief Helper function for mqtt_evt_handler when message of
 * type MQTT_EVT_PUBLISH is sent
 *
 * @param[in] payload: payload received
 * @param[in] payload_size: size of the payload
 * @param[in] topic: topic on which message was received
 * @param[in] topic_len: length of the topic
 * @param[in] next_msg: next message
 * @param[in] next_msg_size: size of next message
 *
 * @note: This function runs in interrupt context for mqtt. Make
 * sure it is not performing heavy tasks.
 */
enum oob_messages thingsboard_process_message(uint8_t *payload,
					      uint32_t payload_length,
					      uint8_t *topic,
					      uint32_t topic_length,
					      char *next_msg,
					      size_t next_msg_size);


#endif

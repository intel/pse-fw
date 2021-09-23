/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file azure_iot.h
 *
 * \brief This file contains declarations for AZURE_IOT specific calls which get
 * are used in adapter.c
 *
 */

#ifndef _AZURE_IOT_H_
#define _AZURE_IOT_H_

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
#define AZURE_IOT_TLS_SNI_HOSTNAME "*.azure-devices.net" /*"*.cloudapp.net"*/
#endif


/** No of azure_iot subscription topics */
#define NUM_AZURE_IOT_SUB_TOPICS 1

/** AZURE_IOT specific messages keys known as params */
#define AZURE_IOT_REBOOT_PARAMS_METHOD "reboot_device"
#define AZURE_IOT_DECOMMISSION_PARAMS_METHOD "decommision_device"
#define AZURE_IOT_POWERDOWN_PARAMS_METHOD "powerdown_device"
#define AZURE_IOT_POWERUP_PARAMS_METHOD "powerup_device"
#define AZURE_IOT_METHOD_EXEC "method.exec"

/** AZURE_IOT specific messages */
#define AZURE_IOT_REQUEST_ID "rid"
#define AZURE_IOT_EVENT_GENERIC "event_generic"

/** Event message format: {"key":"value"} */
#define AZURE_IOT_EVENT_MESSAGE_TEMPLATE \
	"{\"%s\":\"%s\"}"

/** Attribute message format: {"key":"value"} */
#define AZURE_IOT_ATTRIBUTE_MESSAGE_TEMPLATE \
	"{\"%s\":\"%s\"}"

/** AZURE_IOT publish topic.. All outgoing messages are published on this topic */
#define AZURE_IOT_MQTT_PUB_ATTRIB_TOPIC "$iothub/twin/PATCH/properties/reported/"
#define AZURE_IOT_MQTT_PUB_TELEM_TOPIC "devices/%s/messages/events/"

/** AZURE_IOT topic on which server post most replies */
#define AZURE_IOT_RPC_REPLY_TOPIC "$iothub/methods/res/201/?$%s"

/** AZURE_IOT topic on which server posts messages for every trigger */
#define AZURE_IOT_RPC_SUB_TOPIC "$iothub/methods/POST/#"

/**
 * AZURE_IOT specific method to get mqtt subscription topics count
 *
 * @retval int count of topics
 **/
int azure_iot_get_mqtt_sub_topics_count(void);

/**
 * AZURE_IOT specific method to get mqtt subscription topics
 *
 * @param [in][out] char sub_topics[][]
 *      Uses No.of sub topics and their max lengths.
 *
 * @retval OOB_ERR_ADAPTER_SUB_TOPIC_CPY on failure , OOB_SUCCESS otherwise
 **/
int azure_iot_get_mqtt_sub_topics(
	char sub_topics[NUM_AZURE_IOT_SUB_TOPICS][MAX_MQTT_SUBS_TOPIC_LEN]);

/**
 * AZURE_IOT specific method to get mqtt publish topic
 *
 * @param [in][out] enum app_message_type
 * @retval char *
 **/
char *azure_iot_get_mqtt_pub_topic(enum app_message_type pub_msg_type);


/**
 * AZURE_IOT specific publish telemetry topic w.r.t
 * specific mqtt client id populated.
 *
 * @param [in][out] msg_buf char *
 * @param [in]      msg_buf_size size_t
 * @param [in]      const value * value for the client id
 * @retval void
 **/
void azure_iot_prepare_pub_telem_topic(char *msg_buf,
				       size_t msg_buf_size,
				       const char *value);
/**
 * AZURE_IOT specific call to create an attribute message
 * for static telemetry publish request. Static telemetry
 * is called an attribute in AZURE_IOT
 *
 * @param [in][out] msg_buf char *
 * @param [in]      msg_buf_size size_t
 * @param [in]      key const char * Key for the attribute
 * @param [in]      key const value * value for that key
 * @retval void
 **/
void azure_iot_prepare_attrib_pub_message(char *msg_buf,
					  size_t msg_buf_size,
					  const char *key,
					  const char *value);

/**
 * AZURE_IOT specific call to create a event message depending
 * on the parameters passed. An event message is
 * anything that needs to notified to the server. Event is sometimes
 * called a log event in AZURE_IOT
 *
 * @param [in][out] msg_buf char *
 * @param [in]      msg_buf size_t
 * @param [in]      message const char * event message to be passed
 * @retval void
 **/
void azure_iot_prepare_log_message(char *msg_buf, size_t msg_buf_size,
				   const char *message);

/**
 * AZURE_IOT specific call to send acknowledgment on successful
 * completion of method execution.
 *
 * @param [in]      payload: payload received
 * @param [in][out] msg_buf: topic message to be passed
 * @param [in]      msg_buf_size: size of the topic
 * @retval void
 **/
void azure_iot_prepare_pub_ack_topic(char *payload, char *msg_buf,
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
enum oob_messages azure_iot_process_message(uint8_t *payload,
					    uint32_t payload_length,
					    uint8_t *topic,
					    uint32_t topic_length,
					    char *next_msg,
					    size_t next_msg_size);

#endif


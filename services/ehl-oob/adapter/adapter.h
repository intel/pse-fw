/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file adapter.h
 *
 * \brief This file contains all the interfaces to cloud adapter
 * functions. The functions like preparing messages of type
 * static, dynamic telemetry, event, subscription and processing incoming
 * messages have to be defined here. Calls to these functions will ultimately
 * be delegated to cloud adapter specific functions depending upon the adapter
 * set.
 *
 * \note Make sure to hook your cloud adapter specific header file.
 *
 */

#ifndef _ADAPTER_H_
#define _ADAPTER_H_

#include <zephyr.h>
#include <common/utils.h>

#if defined(CONFIG_MQTT_LIB_TLS) || defined(CONFIG_MQTT_LIB)
#include <mqtt_client/mqtt_client.h>

/**
 * Function pointer type to call mqtt publish topic
 *
 * @retval char *
 **/
typedef char *(*mqtt_pub_topic)(enum app_message_type pub_msg_type);

/**
 * Function pointer type to call mqtt subscription topics count
 *
 * @retval int no of subscription topics
 **/
typedef int (*mqtt_sub_topics_count)(void);

/**
 * Function pointer type to get mqtt subscription topics
 *
 * @param [in][out] const sub_topics char *
 * @retval OOB_ERR_ADAPTER_SUB_TOPIC_CPY on failure, OOB_SUCCESS otherwise
 **/
typedef int (*mqtt_sub_topics)(char sub_topics[][MAX_MQTT_SUBS_TOPIC_LEN]);

/**
 * Function pointer type to create a static telemetry attribute message
 * depending on the parameters passed.
 *
 * @param [in][out] msg_buf char *
 * @param [in]      msg_buf_size size_t
 * @param [in]      key const char *
 * @param [in]      key const value *
 * @retval void
 **/
typedef void (*mqtt_static_attrib_pub_msg)(char *msg_buf,
					   size_t msg_buf_size,
					   const char *key,
					   const char *value);

/**
 * Function pointer type to create a event message depending
 * on the parameters passed. An event message is
 * anything that needs to notified to the server thats
 * occurring on the device.
 *
 * @param [in][out] msg_buf char *
 * @param [in]      msg_buf_size size_t
 * @param [in]      message const char *
 * @retval void
 **/
typedef void (*mqtt_event_pub_msg)(char *msg_buf,
				   size_t msg_buf_size,
				   const char *message);

/**
 * Function pointer type to process incoming messages from
 * evt_handler of messaging protocols like mqtt etc.
 *
 * @param[in] payload: payload received
 * @param[in] payload_size: size of the payload
 * @param[in] topic: topic on which message was received
 * @param[in] topic_len: length of the topic
 *
 * @note: This function runs in interrupt context for mqtt. Make
 * sure it is not performing heavy tasks.
 */
typedef enum oob_messages (*mqtt_process_incoming_message)(uint8_t *payload,
							   uint32_t payload_length,
							   uint8_t *topic,
							   uint32_t topic_length,
							   char *next_msg,
							   size_t next_msg_size);

/**
 * Function which performs function pointers assignment from
 * specific adapter functions
 *
 * @param[in] void
 *
 * @parm[out] void
 */
void initialize_cloud_adapter(void);

/**
 * Structure which is used in messaging protocols
 * to call appropriate calls for different cloud adapter
 *
 * These function pointers are assigned in adapter.c depending
 * on the chosen adapter.
 */
struct cloud_adapter_client {
	mqtt_pub_topic get_mqtt_pub_topic;
	mqtt_sub_topics_count get_mqtt_sub_topics_count;
	mqtt_sub_topics get_mqtt_sub_topics;
	mqtt_static_attrib_pub_msg prep_static_attrib_pub_msg;
	mqtt_event_pub_msg prep_event_pub_msg;
	mqtt_process_incoming_message process_message;
};

#endif /* defined CONFIG_MQTT_LIB_TLS OR CONFIG_MQTT_LIB */

extern struct cloud_adapter_client cloud_adapter;

#endif

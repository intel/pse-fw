/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file adapter.c
 *
 * \brief This file contains bascially performs the function pointer assignments
 * to cloud adapter functions. eg.: Here since the adapter is telit, we perform
 * assignment of telit functions to function pointers.
 *
 * \note You might need to define your own function pointer types if you want
 * to use a different messaging protocol like LWM2M for telit or other
 *
 * \see adapter.h
 *
 */

#include "adapter.h"
#include <stdio.h>
#include "telit.h"
#include "thingsboard.h"
#include "azure_iot.h"
#include <common/credentials.h>
#include <common/utils.h>
#include <common/pse_app_framework.h>

VAR_DEFINER struct cloud_adapter_client cloud_adapter;
extern VAR_DEFINER struct cloud_credentials *creds;

/** Function which performs function pointers
 * assignment from specific adapter functions.
 *
 * If you are writing an new adapter and using mqtt, be sure to include
 * your header file inside adapter.h. You wouldn't have to
 * modify your mqtt function pointer types or the cloud_adapter
 * structure. In that case, only change this function
 * to reassign those function pointers.
 *
 * If you writing a new adapter and using a protocol other than mqtt,
 * you need to define your own function pointer types & modify the
 * struct cloud_adapter_client in adapter.h
 */
void initialize_cloud_adapter(void)
{
	if (creds->cloud_adapter == TELIT) {
#if defined(CONFIG_MQTT_LIB_TLS) || defined(CONFIG_MQTT_LIB)
		cloud_adapter.get_mqtt_pub_topic =
			telit_get_mqtt_pub_topic;

		cloud_adapter.get_mqtt_sub_topics_count =
			telit_get_mqtt_sub_topics_count;

		cloud_adapter.get_mqtt_sub_topics =
			telit_get_mqtt_sub_topics;

		cloud_adapter.prep_static_attrib_pub_msg =
			telit_prepare_attrib_pub_message;

		cloud_adapter.prep_event_pub_msg =
			telit_prepare_log_message;

		cloud_adapter.process_message =
			telit_process_message;
#endif
	} else if (creds->cloud_adapter == THINGSBOARD) {
#if defined(CONFIG_MQTT_LIB_TLS) || defined(CONFIG_MQTT_LIB)
		cloud_adapter.get_mqtt_pub_topic =
			thingsboard_get_mqtt_pub_topic;

		cloud_adapter.get_mqtt_sub_topics_count =
			thingsboard_get_mqtt_sub_topics_count;

		cloud_adapter.get_mqtt_sub_topics =
			thingsboard_get_mqtt_sub_topics;

		cloud_adapter.prep_static_attrib_pub_msg =
			thingsboard_prepare_attrib_pub_message;

		cloud_adapter.prep_event_pub_msg =
			thingsboard_prepare_log_message;

		cloud_adapter.process_message =
			thingsboard_process_message;
#endif
	} else if (creds->cloud_adapter == AZURE) {
#if defined(CONFIG_MQTT_LIB_TLS) || defined(CONFIG_MQTT_LIB)
		cloud_adapter.get_mqtt_pub_topic =
			azure_iot_get_mqtt_pub_topic;

		cloud_adapter.get_mqtt_sub_topics_count =
			azure_iot_get_mqtt_sub_topics_count;

		cloud_adapter.get_mqtt_sub_topics =
			azure_iot_get_mqtt_sub_topics;

		cloud_adapter.prep_static_attrib_pub_msg =
			azure_iot_prepare_attrib_pub_message;

		cloud_adapter.prep_event_pub_msg =
			azure_iot_prepare_log_message;

		cloud_adapter.process_message =
			azure_iot_process_message;
#endif
	}
}

/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file oob_errors.h
 *
 * \brief This contains declaration for all the structures and variables
 * needed by managability to establish connection with the cloud.
 *
 */

#ifndef _OOB_ERRORS_H_
#define _OOB_ERRORS_H_

/**
 * OOB ERROR CODES
 */

/* OOB SUCCESS codes*/
#define OOB_SUCCESS 0

/* Use to make OOB thread entry point exit*/
#define OOB_ERR_THREAD_ABORT 1

/* OOB Network Interface found */
#define OOB_ERR_NO_INTERFACE_FOUND 2

/* OOB TLS credentials add err */
#define OOB_ERR_TLS_CREDENTIALS_ADD 3

/* OOB PMC client related errors */
#define OOB_ERR_PMC_FAILED 4
#define OOB_ERR_PMC_INVALID_ACTION_ARGUMENT 5

/* OOB Buffer related */
#define OOB_ERR_BUFFER_OVERFLOW 6
#define OOB_ERR_MESSAGE_FAILED 7

/* OOB cloud credentials fetch */
#define OOB_ERR_FETCH_ERROR 8

#define OOB_ERR_FETCH_CLOUD_ADAPTER 9
#define OOB_ERR_FETCH_CLOUD_USERNAME 10
#define OOB_ERR_FETCH_CLOUD_PASSWORD 11
#define OOB_ERR_FETCH_CLOUD_HOST_URL 12
#define OOB_ERR_FETCH_CLOUD_HOST_PORT 13
#define OOB_ERR_FETCH_PROXY_HOST_URL 14
#define OOB_ERR_FETCH_PROXY_HOST_PORT 15
#define OOB_ERR_FETCH_ROOT_CA 16
#define OOB_ERR_FETCH_CLOUD_MQTT_CLIENT_ID 17

/* OOB memory allocation */
#define OOB_ERR_MALLOC 18

/* sub topic, subscription error */
#define OOB_ERR_ADAPTER_SUB_TOPIC_CPY 19
#define OOB_ERR_MQTT_SUBSCRIPTION_ERR 20

/* OOB Network errors */
#define OOB_ERR_DNS_RESOLUTION_ERR 21
#define OOB_ERR_NO_IPV4_ADDR 22
#define OOB_DECOMM_COMPLETED 23

/* OOB MQTT connection error */
#define OOB_ERR_MQTT_DISCONNECT 24
#define OOB_ERR_MQTT_RECONNECT 25

#endif  /* END _OOB_ERRORS_H_ */

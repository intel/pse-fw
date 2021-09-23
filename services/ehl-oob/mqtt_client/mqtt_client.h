/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file mqtt_client.h
 *
 * \brief This contains the MQTT headers needed by ehl-oob.
 *
 */

#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_

#include <common/utils.h>
#include <net/mqtt.h>
#include <net/socket.h>

#ifdef CONFIG_MQTT_LIB_TLS
#include <mbedtls/ssl_ciphersuites.h>
#endif /* endif CONFIG_MQTT_LIB_TLS */

/* Global fifo structure */
extern VAR_DEFINER_BSS struct k_fifo managability_fifo;
/* Global to keep track of mqtt_connection */
extern VAR_DEFINER_BSS bool connected;
/* Global to keep track of TLS session state */
extern VAR_DEFINER_BSS enum oob_conn_state oob_conn_st;
/** The mqtt client struct */
extern VAR_DEFINER_BSS struct mqtt_client client;


/** [Non-TLS settings] */
/** Millisecs before ping_req_timer expires **/
#define MQTT_PING_INTERVAL      8000

#define MQTT_CONNECTED(rc)      ((rc) == 0 ? "MQTT DISCONNECTED" : "CONNECTED")
#define PRINT_RESULT(func, rc)				     \
	LOG_DBG("[%s:%d] %s: %d <%s>\n", __func__, __LINE__, \
		(func), rc, RC_STR(rc))

/** Millisecs sleep time between messages for MQTT */
#define APP_SLEEP_MSECS         1000

/* Buffer size for MQTT to pass as buffers
 * to underlying net layer
 */
#define SMALL_BUFFER            128
#define MEDIUM_BUFFER           256
#define LARGE_BUFFER            512
#define XL_LARGE_BUFFER         1024

/** [TLS Settings] */
#ifdef CONFIG_MQTT_LIB_TLS
/** SECURE TLS */
#define SERVER_PORT             8883
/* TLS Peer verify required*/
#define OOB_TLS_PEER_VERIFY     2
#else
/** INSECURE */
#define SERVER_PORT             1883
#endif  /* endif CONFIG_MQTT_LIB_TLS */

/** PROXY PORT */
#if defined(CONFIG_SOCKS)
#define PROXY_PORT             1080
#endif

/** Max length for MQTT_SUB_TOPIC */
#define MAX_MQTT_SUBS_TOPIC_LEN         200

/** Function thats gets called from ehl_oob_main to send messages of type
 * EVENT, API. Calls lower level functions publish_event, publish_api
 *
 * @param [in] payload char pointer
 * @param [in] type enum app_message_type
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 * @retval -EIO
 */
int post_message(char *payload, enum app_message_type type);

/** Function thats gets called from ehl_oob_main to send messages of
 * app_message_type: STATIC, DYNAMIC. Calls lower level functions
 * publish_static and publish_dynamic in future
 *
 * @param [in] key char pointer
 * @param [in] value char pointer
 * @param [in] type enum app_message_type
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 * @retval -EIO
 */
int send_telemetry(char *key, char *value, enum app_message_type type);

/** Function device cloud init request
 *
 * @param[in] void
 *
 * @return 0 on success
 * @return OOB_ERR_TLS_CREDENTIALS_ADD on failure
 */
int device_cloud_init(void);

/** Function device cloud connect request
 *
 * @param[in] void
 *
 * @return 0 on success
 * @return OOB_ERR_DNS_RESOLUTION_ERR
 * @return OOB_ERR_MQTT_SUBSCRIPTION_ERR
 * @return OOB_ERR_MALLOC
 */
int device_cloud_connect(void);

/** Function device cloud abort request
 *
 * @param[in] void
 *
 * @return 0 on success
 * @return OOB_ERR_MQTT_DISCONNECT
 */
int device_cloud_abort(void);

#endif

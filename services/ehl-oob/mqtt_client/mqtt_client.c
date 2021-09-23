/*
 * Copyright (c) 20 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file mqtt_client.c
 *
 * \brief This contains the MQTT implementation
 *  needed by ehl-oob.
 *
 */

#include <random/rand32.h>
#include <adapter/adapter.h>
#include "common/credentials.h"
#include <common/pse_app_framework.h>
#include "mqtt_client.h"
#include "common/utils.h"
#include <logging/log.h>

LOG_MODULE_REGISTER(OOB_MQTT_CLIENT, CONFIG_OOB_LOGGING);

#define MAX_SUBLIST_COUNT 3

/** delayed work queue to set ping requests */
void timer_expiry_pingreq(struct k_timer *timer);
K_TIMER_DEFINE(ping_req_timer, timer_expiry_pingreq, NULL);

/** Global to keep TLS session & mqtt states */
VAR_DEFINER_BSS enum oob_conn_state oob_conn_st;
static VAR_DEFINER_BSS struct fifo_message mqtt_state_q, fmessage;

/** FIFO queue to share messages between protocol and ehl-oob main */
K_FIFO_DEFINE(managability_fifo);

/** The mqtt client struct */
VAR_DEFINER_BSS struct mqtt_client client;

/** Buffers for MQTT client. */
VAR_DEFINER_BSS struct mqtt_utf8 password;
VAR_DEFINER_BSS struct mqtt_utf8 username;

VAR_DEFINER_BSS uint8_t rx_buffer[XL_LARGE_BUFFER];
VAR_DEFINER_BSS uint8_t tx_buffer[XL_LARGE_BUFFER];

/** MQTT Broker details. */
VAR_DEFINER_BSS struct sockaddr_storage broker;

/** Poll structures */
VAR_DEFINER_BSS struct zsock_pollfd fds[1];
VAR_DEFINER_BSS int nfds;

/** Global to keep track of mqtt_connection */
VAR_DEFINER_BSS bool connected;

/** DNS addr info */
#if defined(CONFIG_DNS_RESOLVER)
VAR_DEFINER_BSS struct zsock_addrinfo *haddr;
VAR_DEFINER_BSS struct zsock_addrinfo hints;
#endif
/* Keep track for DNS status */
static VAR_DEFINER_BSS bool dns_status;

VAR_DEFINER_BSS struct mqtt_publish_param param;

VAR_DEFINER_BSS struct mqtt_subscription_list sub_list;
VAR_DEFINER_BSS struct mqtt_topic topic_list[MAX_SUBLIST_COUNT];
VAR_DEFINER_BSS struct mqtt_subscription_list *sub_list_p;

VAR_DEFINER_BSS char msg_buf[BIG_GENERAL_BUF_SIZE];
VAR_DEFINER_BSS char sub_topics[MAX_SUBLIST_COUNT][MAX_MQTT_SUBS_TOPIC_LEN];

VAR_DEFINER_BSS char next_msg[BIG_GENERAL_BUF_SIZE];

#if defined(CONFIG_NET_IPV6)
VAR_DEFINER_BSS struct sockaddr_in6 *broker6;
#else
VAR_DEFINER_BSS struct sockaddr_in *broker4;
#endif

#if defined(CONFIG_SOCKS)
VAR_DEFINER_BSS struct sockaddr socks5_proxy;
VAR_DEFINER_BSS struct sockaddr_in *proxy4;
#endif

/** All MQTT TLS declarations */
#if defined(CONFIG_MQTT_LIB_TLS)

/** Security tag for Certificates */
#define APP_CA_CERT_TAG 1

VAR_DEFINER_BSS struct mqtt_sec_config *tls_config;

/** Global to keep track of TLS_CREDENTIALS for zephyrproject
 * while calling tls_credential_add
 *
 * It is assigned with numeric value of type sec_tag_t, called a tag.
 * This value can be used later on to reference the credential during
 * secure socket configuration with socket options.
 */
VAR_DEFINER sec_tag_t m_sec_tags[] = {
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	APP_CA_CERT_TAG,
#endif
};

/** Function to initialize TLS credentials for
 * mqtt transport tls. We can pass either of the below
 * to the tls_credentail_add. If you add certificates
 * make they are in DER format by default else
 * PEM support can be enabled in mbedTLS settings
 *
 * TLS_CREDENTIAL_CA_CERTIFICATE,
 * TLS_CREDENTIAL_SERVER_CERTIFICATE,
 * TLS_CREDENTIAL_PRIVATE_KEY
 * TLS_CREDENTIAL_PSK OR TLS_CREDENTIAL_PSK_ID
 */
static int tls_init(void)
{
	int err = -EINVAL;

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	err = tls_credential_delete(APP_CA_CERT_TAG,
				    TLS_CREDENTIAL_CA_CERTIFICATE);
	if (err == -ENOENT) {
		LOG_INF("Init Public certificate");
	}

	err = tls_credential_add(APP_CA_CERT_TAG,
				 TLS_CREDENTIAL_CA_CERTIFICATE,
				 creds->trusted_ca,
				 creds->trusted_ca_size);
	if (err < 0) {
		LOG_ERR("Cert didn't work");
		LOG_ERR("Failed to register public certificate: %d", err);
		return err;
	}
#endif
	return err;
}
#endif /* CONFIG_MQTT_LIB_TLS */

/** Sets the file descriptor used by mqtt socket connection
 * depending the transport chosen
 *
 * @param [in] pointer to struct mqtt_client
 */
static void prepare_fds(struct mqtt_client *client)
{
	if (client->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds[0].fd = client->transport.tcp.sock;
	}
#if defined(CONFIG_MQTT_LIB_TLS)
	else if (client->transport.type == MQTT_TRANSPORT_SECURE) {
		fds[0].fd = client->transport.tls.sock;
	}
#endif

	fds[0].events = ZSOCK_POLLIN;
	nfds = 1;
}

/** Clears the file descriptor count. It
 * is typically called after mqtt_disconnect.
 */
static void clear_fds(void)
{
	nfds = 0;
}

/** Perform a net level socket poll.
 * It should typically be called with a suitable timeout
 * before calling any mqtt_apis
 *
 * @param [in] millisecs timeout for poll
 */
static int wait(int timeout)
{
	int ret = 0;

	if (nfds > 0) {
		ret = zsock_poll(fds, nfds, timeout);
		if (ret < 0) {
			LOG_ERR("poll error: %d\n", errno);
		}
	}

	return ret;
}

/** Timer expiry function that gets called once the timer
 * expires every MQTT_PING_INTERVAL
 *
 * @param struct k_timer pointer
 * @retval void.
 */
void timer_expiry_pingreq(struct k_timer *timer)
{
	int ret;

	mqtt_state_q.current_msg_type = MQTT_STATE_CHANGE_EVENT;
	ret = k_fifo_alloc_put(&managability_fifo, &mqtt_state_q);
	if (ret) {
		LOG_ERR("Mqtt state event fifo allocation failed");
	}
}

/**
 * @brief Helper function for mqtt_evt_handler, when message of
 * type MQTT_EVT_PUBLISH is sent by the MQTT Broker
 *
 * @param[in] payload: payload received
 * @param[in] payload_size: size of the payload
 * @param[in] topic: topic on which message was received
 * @param[in] topic_len: length of the topic
 *
 * @note: This function runs in interrupt context. Make
 * sure it is not performing heavy tasks.
 */
static void mqtt_process_receive(uint8_t *payload,
				 uint32_t payload_size,
				 uint8_t *topic,
				 uint32_t topic_len)
{
	enum oob_messages ret = IGNORE;
	int res;

	next_msg[BIG_GENERAL_BUF_SIZE - 1] = '\0';
	size_t next_msg_size = sizeof(next_msg);

	ret = cloud_adapter.process_message(payload,
					    payload_size,
					    topic,
					    topic_len,
					    next_msg,
					    next_msg_size);
	if (ret != IGNORE) {
		fmessage.next_msg = next_msg;
		fmessage.current_msg_type = ret;
		res = k_fifo_alloc_put(&managability_fifo, &fmessage);
		if (res != 0) {
			LOG_ERR("k_fifo_alloc_put failed, ret = %d\n", res);
		}
	}
}

/**
 * @brief Asynchronous event notification callback registered by the
 *        application.
 *
 * @param[in] client Identifies the client for which the event is notified
 * @param[in] evt Event description along with result and associated
 *                parameters (if any).
 *
 * @note: Not all MQTT evts are notified to the application.
 */
void mqtt_evt_handler(struct mqtt_client *const client,
		      const struct mqtt_evt *evt)
{
	int err;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			LOG_DBG("MQTT connect failed %d\n",
				evt->result);
			break;
		}

		connected = true;
		LOG_DBG("[%s:%d] MQTT client connected!\n",
			__func__, __LINE__);

		break;

	case MQTT_EVT_DISCONNECT:
		LOG_DBG("[%s:%d] MQTT client disconnected %d\n",
			__func__,
			__LINE__, evt->result);

		connected = false;
		clear_fds();

		break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_DBG("MQTT PUBACK error %d\n", evt->result);
			break;
		}

		LOG_DBG("[%s:%d] PUBACK packet id: %u\n",
			__func__, __LINE__,
			evt->param.puback.message_id);

		break;

	case MQTT_EVT_PUBREC:
		if (evt->result != 0) {
			LOG_DBG("MQTT PUBREC error %d\n", evt->result);
			break;
		}

		LOG_DBG("[%s:%d] PUBREC packet id: %u\n",
			__func__, __LINE__,
			evt->param.pubrec.message_id);

		const struct mqtt_pubrel_param rel_param = {
			.message_id = evt->param.pubrec.message_id
		};

		err = mqtt_publish_qos2_release(client, &rel_param);
		if (err != 0) {
			LOG_DBG("Failed to send MQTT PUBREL: %d\n", err);
		}

		break;

	case MQTT_EVT_PUBCOMP:
		if (evt->result != 0) {
			LOG_DBG("MQTT PUBCOMP error %d\n", evt->result);
			break;
		}

		LOG_DBG("[%s:%d] PUBCOMP packet id: %u\n",
			__func__, __LINE__,
			evt->param.pubcomp.message_id);

		break;

	case MQTT_EVT_PUBLISH:
		if (evt->result != 0) {
			LOG_DBG("MQTT EVT_PUBLISH error %d\n", evt->result);
			break;
		}

		uint8_t buf[XL_LARGE_BUFFER];
		int rc_mqtt_read = -1;

		memset(buf, 0x00, XL_LARGE_BUFFER);

		LOG_DBG("[%s:%d] EVT_PUBLISH packet id: %u\n",
			__func__, __LINE__,
			evt->param.pubcomp.message_id);

		do {
			rc_mqtt_read = mqtt_read_publish_payload(
				client,
				buf,
				evt->param.publish.message.payload.len);
		} while (rc_mqtt_read > 0);

		buf[evt->param.publish.message.payload.len] = '\0';

		LOG_INF(
			"MQTT callback received message: %s on topic: %s",
			log_strdup(buf),
			log_strdup(evt->param.publish.message.topic.topic.utf8));

		mqtt_process_receive(
			buf,
			(uint32_t)evt->param.publish.message.payload.len,
			(uint8_t *)evt->param.publish.message.topic.topic.utf8,
			(uint32_t)evt->param.publish.message.topic.topic.size
			);

		memset((uint8_t *)evt->param.publish.message.topic.topic.utf8,
		       0x00,
		       (uint32_t)evt->param.publish.message.topic.topic.size);

		break;

	default:
		break;
	}
}

/** Inner function to prepare messages that the application publishes
 * The application passed a pre-allocated buffer to cloud adapter
 * and packages it in mqtt struct mqtt_publish_msg
 *
 * @param [in] pointer to struct mqtt_publish_param
 * @param [in] key char pointer
 * @param [in] value char pointer
 * @param [in] eventmsg char pointer
 * @param [in] api_msg char pointer
 * @param [in] type char pointer
 * @retval non-zero on errro
 */
static int prepare_mqtt_pub_msg(struct mqtt_publish_param *param,
				const char *key,
				const char *value,
				const char *eventmsg,
				const char *api_msg,
				enum app_message_type type)
{

	size_t msg_buf_size = sizeof(char) * BIG_GENERAL_BUF_SIZE;

	if (type == STATIC) {
		cloud_adapter.prep_static_attrib_pub_msg(msg_buf,
							 msg_buf_size,
							 key, value);
	} else if (type == EVENT) {
		cloud_adapter.prep_event_pub_msg(msg_buf,
						 msg_buf_size,
						 eventmsg);
	} else if (type == API) {

		/* Overflow check-prevention*/
		size_t api_msg_size = strlen(api_msg) + 1;

		if (api_msg_size > msg_buf_size) {
			LOG_ERR(
				"Src buffer api_msg size:%d bigger than target size:%d",
				api_msg_size,
				msg_buf_size);
			return OOB_ERR_BUFFER_OVERFLOW;
		}
		memcpy(msg_buf, api_msg, api_msg_size);
		msg_buf[msg_buf_size - 1] = '\0';
	}

	(param->message).topic.qos = MQTT_QOS_1_AT_LEAST_ONCE;
	(param->message).topic.topic.utf8 =
		(uint8_t *)cloud_adapter.get_mqtt_pub_topic(type);
	(param->message).topic.topic.size =
		(uint32_t)strlen(
			(uint8_t *)cloud_adapter.get_mqtt_pub_topic(type));
	(param->message).payload.data = (uint8_t *)(msg_buf);
	(param->message).payload.len =
		(uint32_t)strlen((param->message).payload.data);
	param->message_id = sys_rand32_get();
	param->dup_flag = 0;
	param->retain_flag = 0;

	return OOB_SUCCESS;
}

/** Inner helper function thats gets called by send
 * if app_message_type is EVENT. Its job to package an event
 * message in a mqtt_publish_param and send it by calling
 * mqtt_publish api
 *
 * @param [in] msg char pointer
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 * @retval -EIO
 */
int publish_event(char *msg)
{
	int rc;

	memset(&param, 0x00, sizeof(struct mqtt_publish_param));

	rc = prepare_mqtt_pub_msg(&param,
				  NULL,
				  NULL,
				  msg,
				  NULL,
				  EVENT);

	if (rc != OOB_SUCCESS) {
		return rc;
	}

	LOG_DBG("Publishing event: %s with message_id: %u",
		msg,
		param.message_id);

	rc = mqtt_publish(&client, &param);
	PRINT_RESULT("mqtt_publish", rc);

	if (rc != OOB_SUCCESS) {
		LOG_ERR("mqtt_publish error: %d", rc);
		return rc;
	}

	wait(APP_SLEEP_MSECS);
	rc = mqtt_input(&client);
	return rc;
}

/** Inner helper function thats gets called by send
 * if app_message_type is API. Its job to package an api type
 * message in a mqtt_publish_param & send it by calling
 * mqtt_publish api
 *
 * @param [in] msg char pointer
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 * @retval -EIO
 */
int publish_api(char *msg)
{
	int rc;

	memset(&param, 0x00, sizeof(struct mqtt_publish_param));

	rc = prepare_mqtt_pub_msg(&param,
				  NULL,
				  NULL,
				  NULL,
				  msg,
				  API);

	if (rc != OOB_SUCCESS) {
		return rc;
	}

	LOG_DBG("Publishing api: %s with message_id: %u",
		msg,
		param.message_id);

	rc = mqtt_publish(&client, &param);
	PRINT_RESULT("mqtt_publish", rc);

	if (rc != OOB_SUCCESS) {
		LOG_ERR("mqtt_publish error: %d", rc);
		return rc;
	}

	wait(APP_SLEEP_MSECS);
	rc = mqtt_input(&client);
	return rc;
}

/** Inner helper function thats gets called by send_telemetry
 * if app_message_type is STATIC. Its job to package a static telemetry
 * in struct mqtt_publish and send it by calling mqtt_publish api
 *
 * @param [in] key char pointer
 * @param [in] value char pointer
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 * @retval -EIO
 */
int publish_static(char *key,
		   char *value)
{
	int rc;

	memset(&param, 0x00, sizeof(struct mqtt_publish_param));

	rc = prepare_mqtt_pub_msg(&param,
				  key,
				  value,
				  NULL,
				  NULL,
				  STATIC);

	if (rc != OOB_SUCCESS) {
		return rc;
	}

	LOG_DBG("Publishing static telmetry: key:<%s, %s> with packet_id: %u",
		key,
		value,
		param.message_id);

	rc = mqtt_publish(&client, &param);
	PRINT_RESULT("mqtt_publish", rc);

	if (rc != OOB_SUCCESS) {
		LOG_ERR("mqtt_publish error: %d", rc);
		return rc;
	}

	wait(APP_SLEEP_MSECS);
	rc = mqtt_input(&client);
	return rc;
}

/** Function thats gets called from ehl_oob_main to send messages of type
 * EVENT, API. Calls lower level functions publish_event, publish_api
 *
 * @param [in] payload char pointer
 * @param [in] type enum app_message_type
 *
 * @retval 0 on success
 * @retval -EINVAL from api OR there is no valid app_message_type
 * @retval -ENOMEM
 * @retval -EIO
 */
int post_message(char *payload, enum app_message_type type)
{
	if (type == EVENT) {
		return publish_event(payload);
	} else if (type == API) {
		return publish_api(payload);
	}
	return -EINVAL;
}

/** Function thats gets called from ehl_oob_main to send messages of
 * app_message_type: STATIC, DYNAMIC.
 * Calls lower level functions publish_static and publish_dynamic
 * in future
 *
 * @param [in] key char pointer
 * @param [in] value char pointer
 * @param [in] type enum app_message_type
 *
 * @retval 0 on success
 * @retval -EINVAL from api OR there is no valid app_message_type
 * @retval -ENOMEM
 * @retval -EIO
 */
int send_telemetry(char *key, char *value, enum app_message_type type)
{
	if (type == STATIC) {
		return publish_static(key, value);
	}
	return -EINVAL;
}

/** Function thats gets called from client_init
 * to set the broker context
 */
static void broker_init(void)
{
#if defined(CONFIG_NET_IPV6)
	broker6 = (struct sockset_subscription_topicsaddr_in6 *)&broker;

	broker6->sin6_family = AF_INET6;
	broker6->sin6_port = htons(SERVER_PORT);
	zsock_inet_pton(AF_INET6, creds->cloud_host, &broker6->sin6_addr);
#else
	broker4 = (struct sockaddr_in *)&broker;

	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(SERVER_PORT);
#if defined(CONFIG_DNS_RESOLVER)
	net_ipaddr_copy(&broker4->sin_addr,
			&net_sin(haddr->ai_addr)->sin_addr);
#else
	zsock_inet_pton(AF_INET, creds->cloud_host, &broker4->sin_addr);
#endif
#endif

#if defined(CONFIG_SOCKS)
	proxy4 = (struct sockaddr_in *)&socks5_proxy;
	proxy4->sin_family = AF_INET;
	proxy4->sin_port = htons(PROXY_PORT);
	zsock_inet_pton(AF_INET, creds->proxy_url, &proxy4->sin_addr);
#endif
}

/** Function thats gets called from client_init to get
 * subscription topics from a adapter. We populate
 * a pointer to a mqtt_topic which is received from
 * get_mqtt_sub_topics
 *
 * @param [in][out] sub_list pointer to struct mqtt_subscription_list
 * @retval Success: sub_list pointer to struct mqtt_subscription_list
 *         Failed:  NULL sub_list
 */
struct mqtt_subscription_list *set_subscription_topics(void)
{
	int sub_list_count = cloud_adapter.get_mqtt_sub_topics_count();
	int status, len;

	memset(&sub_list, 0x00, sizeof(sub_list));
	sub_list.list = topic_list;

	memset(sub_topics, '\0', sizeof(sub_topics));
	status = cloud_adapter.get_mqtt_sub_topics(sub_topics);
	if (status == OOB_SUCCESS) {
		sub_list.list_count = sub_list_count;
		sub_list.message_id = sys_rand32_get();

		for (int i = 0; i < sub_list_count; i++) {
			len = strlen(sub_topics[i]);
			sub_list.list[i].topic.utf8 = (uint8_t *)sub_topics[i];
			sub_list.list[i].topic.size = (uint32_t)len;
			sub_list.list[i].qos = MQTT_QOS_2_EXACTLY_ONCE;
		}
	}

	return &sub_list;
}

/** Function that is called from device_cloud_connect
 * to set the client context with username, password
 * broker details, topics to subscribe etc
 *
 */
static void client_init(void)
{
	broker_init();
	LOG_INF("Device Broker Initiated\n");

	mqtt_client_init(&client);

	/* MQTT client configuration */
	client.broker = &broker;
	client.evt_cb = mqtt_evt_handler;
	client.client_id.utf8 = (uint8_t *)creds->mqtt_client_id;
	client.client_id.size = creds->mqtt_client_id_size;

	password.utf8 = (uint8_t *)creds->token;
	password.size = creds->token_size;
	client.password = &password;

	username.utf8 = (uint8_t *)creds->username;
	username.size = creds->username_size;
	client.user_name = &username;

	client.protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client.rx_buf = rx_buffer;
	client.rx_buf_size = sizeof(rx_buffer);
	client.tx_buf = tx_buffer;
	client.tx_buf_size = sizeof(tx_buffer);

	/* Initialize subscription channels */

	/* MQTT transport configuration */
#if defined(CONFIG_MQTT_LIB_TLS)
	client.transport.type = MQTT_TRANSPORT_SECURE;

	tls_config = &client.transport.tls.config;
	tls_config->peer_verify = TLS_PEER_VERIFY_OPTIONAL;
	tls_config->cipher_list =
		(int *)MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384;
	tls_config->sec_tag_list = m_sec_tags;
	tls_config->sec_tag_count = ARRAY_SIZE(m_sec_tags);
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	tls_config->hostname = (char *)creds->cloud_tls_sni;
#else
	tls_config->hostname = NULL;
#endif
#else
	client.transport.type = MQTT_TRANSPORT_NON_SECURE;
#endif

#if defined(CONFIG_SOCKS)
	mqtt_client_set_proxy(&client, &socks5_proxy,
			      socks5_proxy.sa_family == AF_INET ?
			      sizeof(struct sockaddr_in) :
			      sizeof(struct sockaddr_in6));
#endif
}

/** Function thats gets called from device_cloud_connect to connect to cloud
 * It internally calls setup, initializes mqtt context and loops till
 * mqtt_connect turns connected to true when it received MQTT_EVT_CONNACK
 *
 * In this routine we block until the connected variable is 1
 *
 */
static void try_to_connect(void)
{
	int rc;

	while (!connected) {
		client_init();

		rc = mqtt_connect(&client);
		if (rc != OOB_SUCCESS) {
			LOG_ERR("mqtt_connect error: %d", rc);
			k_sleep(K_MSEC(APP_SLEEP_MSECS));
			continue;
		}

		prepare_fds(&client);

		rc = wait(APP_SLEEP_MSECS);
		if (rc < 0) {
			LOG_ERR("wait error: %d", rc);
			mqtt_abort(&client);
			wait(APP_SLEEP_MSECS);
		}

		mqtt_input(&client);

		if (!connected) {
			LOG_ERR("abort-wait-retry");
			mqtt_abort(&client);
			wait(APP_SLEEP_MSECS);
		}
	}
}

/** Function gets called from device_cloud_connect to get DNS resolved cloud
 * host It internally calls getaddrinfo with host ip and port and loops thrice
 * if unsuccessful return EINVAL, else return OOB_SUCCESS
 */
#if defined(CONFIG_DNS_RESOLVER)
static int get_mqtt_broker_addrinfo(void)
{
	int retries = 3;
	int rc = -EINVAL;

	if (dns_status == true) {
		LOG_INF("DNS resolution already done\n");
		return OOB_SUCCESS;
	}

	while (retries--) {
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = 0;

		rc = zsock_getaddrinfo(creds->cloud_host, creds->cloud_port,
				       &hints, &haddr);
		if (rc == OOB_SUCCESS) {
			LOG_INF("DNS resolved for %s:%s",
				creds->cloud_host,
				creds->cloud_port);

			dns_status = true;
			LOG_INF("DNS Resolution success: %d\n", dns_status);
			zsock_freeaddrinfo(haddr);
			return OOB_SUCCESS;
		}

		LOG_ERR("DNS not resolved for %s:%s, retrying",
			creds->cloud_host,
			creds->cloud_port);
	}

	LOG_INF("DNS Resolution failed : %d\n", dns_status);

	if (haddr != NULL) {
		zsock_freeaddrinfo(haddr);
	}

	return rc;
}
#endif

/** Function thats gets called from ehl_oob_main to connect to cloud
 * It internally calls setup, initializes mqtt context and loops till
 * try_to_connect return OOB_SUCCESS
 */
int device_cloud_connect(void)
{
	int rc;

#if defined(CONFIG_DNS_RESOLVER)
	rc = get_mqtt_broker_addrinfo();
	if (rc != OOB_SUCCESS) {
		LOG_ERR("DNS resolution failed %d", rc);
		return OOB_ERR_DNS_RESOLUTION_ERR;
	}
#endif

	try_to_connect();

	sub_list_p = set_subscription_topics();

	for (int i = 0; i < sub_list_p->list_count; i++) {
		LOG_DBG("Subscribed TOPIC list[%d]: %s", i,
			log_strdup((char *)sub_list_p->list[i].topic.utf8));
	}
	rc = mqtt_subscribe(&client, sub_list_p);
	if (rc != OOB_SUCCESS) {
		LOG_ERR("mqtt subscription error %d", rc);
		return OOB_ERR_MQTT_SUBSCRIPTION_ERR;
	}
	LOG_INF("mqtt subscribe %d", rc);
	wait(APP_SLEEP_MSECS);
	mqtt_input(&client);

	k_timer_start(&ping_req_timer,
		      K_MSEC(MQTT_PING_INTERVAL),
		      K_MSEC(MQTT_PING_INTERVAL));

	return OOB_SUCCESS;
}

/*
 * Function that gets called from ehl_oob_main to disconnect
 * from cloud when network down event detected existing
 * connection should be teardown
 */
int device_cloud_abort(void)
{
	int ret;

	k_timer_stop(&ping_req_timer);

	connected = false;
	clear_fds();

	ret = mqtt_abort(&client);
	if (ret != OOB_SUCCESS) {
		LOG_ERR("mqtt abort failed");
		return OOB_ERR_MQTT_DISCONNECT;
	}

	k_fifo_cancel_wait(&managability_fifo);
	return OOB_SUCCESS;
}

int device_cloud_init(void)
{
	int rc;

#if defined(CONFIG_MQTT_LIB_TLS)
	rc = tls_init();
	if (rc != OOB_SUCCESS) {
		return OOB_ERR_TLS_CREDENTIALS_ADD;
	}

	/* Initiate the TLS session state */
	oob_conn_st = TLS_SESSION_INIT;

#endif

	k_fifo_init(&managability_fifo);
	k_timer_init(&ping_req_timer, timer_expiry_pingreq, NULL);
	LOG_INF("Device Cloud Initiated");

	return OOB_SUCCESS;
}


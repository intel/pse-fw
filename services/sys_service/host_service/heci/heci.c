/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <string.h>
#include <stdlib.h>
#include <sys/util.h>
#include "heci_internal.h"
#include "heci_dma.h"
#include "ipc_helper.h"
#include "host_ipc_service.h"
#include "sedi.h"
#include <user_app_framework/user_app_framework.h>
#include <user_app_framework/user_app_config.h>

#ifndef min
#define min(a, b)		  \
	({ __typeof__(a)_a = (a); \
	__typeof__(b)_b = (b); \
	_a < _b ? _a : _b; })
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(heci, CONFIG_HECI_LOG_LEVEL);

APP_SHARED_VAR heci_device_t heci_dev;
__kernel struct k_mutex dev_lock;
__kernel struct k_sem flow_ctrl_sems[HECI_MAX_NUM_OF_CONNECTIONS];

/* dump heci message for debug support */
#if CONFIG_HECI_MSG_DUMP
void dump_heci_msg(uint32_t drbl, heci_bus_msg_t *msg,
		   bool only_hdr, bool is_in_msg);
#define DUMP_HECI_MSG  dump_heci_msg
#else
#define DUMP_HECI_MSG(...)
#endif

/* there are 2 types of heci message, heci protocol management message and heci
 * client communication message. This function sends heci protocol management
 * message
 */

bool heci_send_proto_msg(uint8_t host_addr, uint8_t fw_addr,
			 bool last_frag, uint8_t *data, uint32_t len)
{
	__ASSERT(data != NULL, "invalid arg: *data\n");
	__ASSERT(len <= HECI_MAX_PAYLOAD_SIZE, "invalid payload size\n");

	int ret;
	uint32_t outbound_drbl;
	uint8_t buf[HECI_IPC_PACKET_SIZE] = { 0 };
	const struct device *dev = device_get_binding("IPC_HOST");

#ifdef CONFIG_SYS_MNG
	ret = mng_host_access_req(HECI_HAL_DEFAULT_TIMEOUT);
	if (ret) {
		LOG_ERR("%s failed to request access to host %d", __func__,
			(uint32_t)ret);
		return false;
	}
#endif

	heci_bus_msg_t *msg = (heci_bus_msg_t *)buf;

	msg->hdr.host_addr = host_addr;
	msg->hdr.fw_addr = fw_addr;
	msg->hdr.last_frag = last_frag ? 1 : 0;
	msg->hdr.len = len;
	memcpy(msg->payload, data, len);
	len += sizeof(msg->hdr);

	outbound_drbl = IPC_BUILD_DRBL(len, IPC_PROTOCOL_HECI);
	ret = ipc_write_msg(dev, outbound_drbl, buf, len, NULL, NULL, 0);

#ifdef CONFIG_SYS_MNG
	mng_host_access_dereq();
#endif

	DUMP_HECI_MSG(outbound_drbl, msg, false, false);
	if (ret) {
		LOG_ERR("write HECI protocol message err");
		return false;
	}

	return true;
}

/* there are 2 types of heci message, heci protocol management message and heci
 * client communication message. This function sends heci client communication
 * message
 */
static bool send_client_msg_ipc(heci_conn_t *conn, mrd_t *msg)
{
	__ASSERT(conn != NULL, "invalid heci connection\n");
	__ASSERT(msg != NULL, "invalid heci client msg to send\n");

	uint8_t buf[HECI_IPC_PACKET_SIZE] = { 0 };
	heci_bus_msg_t *bus_msg = (heci_bus_msg_t *)buf;

	int ret;
	unsigned int fragment_size;
	unsigned int done_bytes = 0;
	uint32_t out_drbl;
	const struct device *dev = device_get_binding("IPC_HOST");
	uint32_t copy_size;

#ifdef CONFIG_SYS_MNG
	ret = mng_host_access_req(HECI_HAL_DEFAULT_TIMEOUT);
	if (ret) {
		LOG_ERR("%s failed to request access to host %d", __func__,
			(uint32_t)ret);
		return false;
	}
#endif

	/* Initialize heci bus message header */
	bus_msg->hdr.host_addr = conn->host_addr;
	bus_msg->hdr.fw_addr = conn->fw_addr;
	bus_msg->hdr.reserved = 0;
	bus_msg->hdr.last_frag = 0;

	while (msg != NULL) {
		fragment_size = 0;
		/* try to copy as much as we can into current fragment */
		while ((fragment_size < HECI_MAX_PAYLOAD_SIZE)
		       && (msg != NULL)) {
			copy_size = min(msg->len - done_bytes,
					HECI_MAX_PAYLOAD_SIZE - fragment_size);

			memcpy(bus_msg->payload + fragment_size,
			       (uint8_t *)msg->buf + done_bytes, copy_size);

			done_bytes += copy_size;
			fragment_size += copy_size;

			if (done_bytes == msg->len) {
				/* continue to next MRD in chain */
				msg = msg->next;
				done_bytes = 0;
			}
		}

		if (msg == NULL) {
			/* this is the last fragment */
			bus_msg->hdr.last_frag = 1;
		}

		bus_msg->hdr.len = fragment_size;
		fragment_size += sizeof(bus_msg->hdr);

		out_drbl = IPC_BUILD_DRBL(fragment_size, IPC_PROTOCOL_HECI);
		DUMP_HECI_MSG(out_drbl, bus_msg, false, false);
		ret = ipc_write_msg(dev, out_drbl, buf,
				    fragment_size, NULL, NULL, 0);
		if (ret) {
#ifdef CONFIG_SYS_MNG
			mng_host_access_dereq();
#endif

			LOG_ERR("write HECI client msg err");
			return false;
		}
	}

#ifdef CONFIG_SYS_MNG
	mng_host_access_dereq();
#endif

	return true;
}

/*
 * wait for host to send flow control to unblock sending task
 * called with dev_lock locked
 */
static bool heci_wait_for_flow_control(heci_conn_t *conn)
{
	int sem_ret;
	bool ret;

	while (true) {
		sem_ret = k_sem_take(conn->flow_ctrl_sem,
				     K_MSEC(
					     CONFIG_HECI_FLOW_CONTROL_WAIT_TIMEOUT));
		k_mutex_lock(&dev_lock, K_FOREVER);
		if (sem_ret) {
			LOG_WRN("heci send timed out");
			ret = false;
			break;
		}

		if (conn->host_buffers) {
			conn->wait_thread_count--;
			ret = true;
			break;
		}

		k_mutex_unlock(&dev_lock);
	}

	k_mutex_unlock(&dev_lock);
	return ret;
}

static inline void heci_wakeup_sender(heci_conn_t *conn, uint8_t num_of_thread)
{
	for (int i = 0; i < num_of_thread; i++) {
		k_sem_give(conn->flow_ctrl_sem);
	}
}

/* calculate heci msg length to send, return minus for invalid sending*/
static int32_t cal_send_msg_len(uint32_t conn_id, mrd_t *msg)
{
	__ASSERT(conn_id < HECI_MAX_NUM_OF_CONNECTIONS,
		 "invalid heci connection\n");
	__ASSERT(msg != NULL, "invalid heci msg\n");

	uint32_t total_len = 0;
	uint32_t max_size;
	const mrd_t *m = msg;
	heci_conn_t *conn;

	k_mutex_lock(&dev_lock, K_FOREVER);

	conn = &heci_dev.connections[conn_id];
	if (!(conn->state & HECI_CONN_STATE_OPEN)) {
		LOG_ERR("bad connection state 0x%x", conn->state);
		k_mutex_unlock(&dev_lock);
		return -1;
	}

	max_size = conn->client->properties.max_msg_size;
	k_mutex_unlock(&dev_lock);

	/* make sure total message length is less than HECI_MAX_MSG_SIZE */
	while (m != NULL) {
		if ((m->len == 0) || (m->buf == NULL)) {
			LOG_ERR("invalid mrd desc: %p, buf: %p len: %u",
				m, m->buf, m->len);
			return -1;
		}

		total_len += m->len;
		if ((total_len > max_size) || (total_len > HECI_MAX_MSG_SIZE)
		    || (m->len > HECI_MAX_MSG_SIZE)) {
			LOG_ERR("too big msg length %u\n", total_len);
			return -EPERM;
		}
		m = m->next;
	}
	return total_len;
}

bool heci_send(uint32_t conn_id, mrd_t *msg)
{
	int32_t total_len;
	bool sent = false;

	total_len = cal_send_msg_len(conn_id, msg);

	if (total_len < 0) {
		return false;
	}
	k_mutex_lock(&dev_lock, K_FOREVER);

	heci_conn_t *conn = &heci_dev.connections[conn_id];

	LOG_INF("heci send message to connection: %d(%d<->%d)",
		conn_id, conn->host_addr, conn->fw_addr);

	if (conn->host_buffers == 0) {
		LOG_DBG("wait for flow control\n");
		conn->wait_thread_count++;
		k_mutex_unlock(&dev_lock);

		if (false == heci_wait_for_flow_control(conn)) {
			return false;
		}
		/* take lock again after getting flow control successfully */
		k_mutex_lock(&dev_lock, K_FOREVER);
	}

#ifdef CONFIG_HECI_USE_DMA
	if ((total_len > CONFIG_HECI_DMA_THRESHOLD)
	    && (conn->client->properties.dma_enabled)) {
		sent = send_client_msg_dma(conn, msg);
	}
#endif
	if (!sent) {
		sent = send_client_msg_ipc(conn, msg);
	}

	if (sent) {
		/* decrease FC credit */
		conn->host_buffers--;
	} else {
		LOG_ERR("heci send fail!");
	}

	k_mutex_unlock(&dev_lock);
	return sent;
}

bool heci_send_flow_control(uint32_t conn_id)
{
	bool ret;
	heci_conn_t *conn;
	heci_flow_ctrl_t fc;

	if (conn_id >= HECI_MAX_NUM_OF_CONNECTIONS) {
		LOG_ERR("bad conn id %u, can't send FC", conn_id);
		return false;
	}

	k_mutex_lock(&dev_lock, K_FOREVER);

	conn = &heci_dev.connections[conn_id];
	if (!(conn->state & HECI_CONN_STATE_OPEN)) {
		LOG_WRN("heci connection %d is closed now, fails to send fc",
			(int)conn_id);
		ret = false;
		goto out_unlock;
	}

	/* free connection rx buffer */
	if (conn->rx_buffer != NULL) {
		heci_rx_msg_t *buf = conn->rx_buffer;

		buf->length = 0;
		buf->type = 0;
		buf->msg_lock = MSG_UNLOCKED;
		conn->rx_buffer = NULL;
	}

	/* build flow control message */
	fc.command = HECI_BUS_MSG_FLOW_CONTROL;
	fc.host_addr = conn->host_addr;
	fc.fw_addr = conn->fw_addr;
	fc.number_of_packets = 1;
	fc.reserved = 0;

	LOG_DBG("to connection: %d(%d<->%d)",
		conn_id, conn->host_addr, conn->fw_addr);

	ret = heci_send_proto_msg(HECI_DRIVER_ADDRESS,
				  HECI_DRIVER_ADDRESS,
				  true, (uint8_t *)&fc, sizeof(fc));

out_unlock:
	k_mutex_unlock(&dev_lock);
	return ret;
}

static inline void heci_notify_client(heci_conn_t *conn, uint32_t event)
{
	if (conn->event_cb) {
		conn->event_cb(event, conn->param);
	}
}

static heci_conn_t *heci_find_conn(uint8_t fw_addr,
				   uint8_t host_addr, uint8_t state)
{
	int i;
	heci_conn_t *conn = &heci_dev.connections[0];

	for (i = 0; i < HECI_MAX_NUM_OF_CONNECTIONS; i++, conn++) {
		if ((conn->state & state) &&
		    (conn->fw_addr == fw_addr) &&
		    (conn->host_addr == host_addr)) {
			return conn;
		}
	}

	return NULL;
}

static void heci_connection_reset(heci_conn_t *conn)
{
	if ((conn == NULL) || (conn->client == NULL)) {
		return;
	}

	conn->state = HECI_CONN_STATE_DISCONNECTING;
	heci_notify_client(conn, HECI_EVENT_DISCONN);
}

static void heci_version_resp(heci_bus_msg_t *msg)
{
	heci_version_req_t *req = (heci_version_req_t *)msg->payload;
	heci_version_resp_t resp = { 0 };

	LOG_DBG("");

	if (msg->hdr.len != sizeof(heci_version_req_t)) {
		LOG_ERR("wrong VERSION_REQ len %d", msg->hdr.len);
		return;
	}

	resp.command = HECI_BUS_MSG_VERSION_RESP;
	resp.major_ver = HECI_DRIVER_MAJOR_VERSION;
	resp.minor_ver = HECI_DRIVER_MINOR_VERSION;

	if ((req->major_ver == HECI_DRIVER_MAJOR_VERSION) &&
	    (req->minor_ver == HECI_DRIVER_MINOR_VERSION)) {
		resp.supported = 1;
	} else {
		resp.supported = 0;
	}

	heci_send_proto_msg(HECI_DRIVER_ADDRESS, HECI_DRIVER_ADDRESS,
			    true, (uint8_t *)&resp, sizeof(resp));

}

static void heci_stop_resp(heci_bus_msg_t *msg)
{
	heci_version_resp_t resp = { 0 };

	LOG_DBG("");

	sedi_fwst_set(ILUP_HOST, 0);
	heci_reset();

	resp.command = HECI_BUS_MSG_HOST_STOP_RESP;
	heci_send_proto_msg(HECI_DRIVER_ADDRESS, HECI_DRIVER_ADDRESS,
			    true, (uint8_t *)&resp, sizeof(resp));
}

static void heci_enum_resp(heci_bus_msg_t *msg)
{
	int i;
	heci_client_ctrl_t *client;
	heci_host_enum_req_t *req = (heci_host_enum_req_t *)msg->payload;
	heci_host_enum_resp_t resp = { 0 };

	LOG_DBG("");

	if (msg->hdr.len != sizeof(heci_host_enum_req_t)) {
		LOG_ERR("wrong ENUM_REQ len %d", msg->hdr.len);
		return;
	}

	client = heci_dev.clients;
	for (i = 0; i < heci_dev.registered_clients; i++, client++) {
		resp.valid_addresses[client->client_addr / BITS_PER_DW] |=
			1 << (client->client_addr % BITS_PER_DW);
		client->active = true;
	}
	resp.command = HECI_BUS_MSG_HOST_ENUM_RESP;

	heci_send_proto_msg(HECI_DRIVER_ADDRESS, HECI_DRIVER_ADDRESS,
			    true, (uint8_t *)&resp, sizeof(resp));

	/*
	 * Setting client_req_bits will allow host to be notified
	 * about new clients
	 */
	if (req->client_req_bits) {
		heci_dev.notify_new_clients = true;
	}

}

static void heci_client_prop(heci_bus_msg_t *msg)
{
	int i;
	heci_client_prop_req_t *req = (heci_client_prop_req_t *)msg->payload;
	heci_client_prop_resp_t resp = { 0 };

	LOG_DBG("");

	if (msg->hdr.len != sizeof(heci_client_prop_req_t)) {
		LOG_ERR("wrong PROP_REQ len %d", msg->hdr.len);
		return;
	}

	for (i = 0; i < HECI_MAX_NUM_OF_CLIENTS; i++) {
		if (heci_dev.clients[i].client_addr == req->address) {
			break;
		}
	}

	resp.command = HECI_BUS_MSG_HOST_CLIENT_PROP_RESP;
	resp.address = req->address;

	if (i == HECI_MAX_NUM_OF_CLIENTS) {
		resp.status = HECI_CONNECT_STATUS_CLIENT_NOT_FOUND;
	} else {
		heci_client_t *client = &heci_dev.clients[i].properties;

		resp.protocol_id = client->protocol_id;
		resp.protocol_ver = client->protocol_ver;
		resp.max_n_of_conns = client->max_n_of_connections;
		resp.max_msg_size = client->max_msg_size;

		resp.dma_header_length = client->dma_header_length;
		resp.dma_enabled = client->dma_enabled;

		resp.status = HECI_CONNECT_STATUS_SUCCESS;
	}

	heci_send_proto_msg(HECI_DRIVER_ADDRESS, HECI_DRIVER_ADDRESS,
			    true, (uint8_t *)&resp, sizeof(resp));

}

static heci_rx_msg_t *heci_get_buffer_from_pool(heci_client_ctrl_t *client)
{
	heci_rx_msg_t *msg;

	if (client == NULL) {
		LOG_ERR("invalid client");
		return NULL;
	}

	msg = client->properties.rx_msg;
	if (msg->msg_lock == MSG_LOCKED) {
		LOG_ERR("client %d no free buf", client->client_addr);
		return NULL;
	}

	msg->msg_lock = MSG_LOCKED;
	return msg;
}

static void heci_connect_host(heci_bus_msg_t *msg)
{
	int client_id;
	int conn_id;
	heci_conn_req_t *req = (heci_conn_req_t *)msg->payload;
	heci_conn_resp_t resp = { 0 };
	heci_conn_t *idle_conn = NULL;
	heci_client_ctrl_t *client;

	if (msg->hdr.len != sizeof(heci_conn_req_t)) {
		LOG_ERR("wrong CONN_REQ len %d", msg->hdr.len);
		return;
	}

	resp.command = HECI_BUS_MSG_CLIENT_CONNECT_RESP;
	resp.fw_addr = req->fw_addr;
	resp.host_addr = req->host_addr;

	/* Try to find the client */
	for (client_id = 0; client_id < HECI_MAX_NUM_OF_CLIENTS; client_id++) {
		client = &heci_dev.clients[client_id];
		if (client->client_addr == req->fw_addr) {
			break;
		}
	}

	if (client_id == HECI_MAX_NUM_OF_CLIENTS) {
		LOG_ERR("conn-client %d not found", req->fw_addr);
		resp.status = HECI_CONNECT_STATUS_CLIENT_NOT_FOUND;
		goto out;
	}

	if (req->host_addr == 0) {
		LOG_ERR("client %d get an invalid host addr 0x%02x", client_id,
			req->host_addr);
		resp.status = HECI_CONNECT_STATUS_REJECTED;
		goto out;
	}

	/*
	 * check whether it's a dynamic client that the host didn't
	 * acknowledge with HECI_BUS_MSG_ADD_CLIENT_RESP message
	 */
	if (!client->active) {
		LOG_ERR("client %d is inactive", req->fw_addr);
		resp.status = HECI_CONNECT_STATUS_INACTIVE_CLIENT;
		goto out;
	}

	if (client->n_of_conns == client->properties.max_n_of_connections) {
		LOG_ERR("client %d exceeds max connection", client_id);
		resp.status = HECI_CONNECT_STATUS_REJECTED;
		goto out;
	}

	/* Look-up among existing dynamic address connections
	 * in order to validate the request
	 */
	for (conn_id = 0; conn_id < HECI_MAX_NUM_OF_CONNECTIONS; conn_id++) {
		heci_conn_t *conn = &heci_dev.connections[conn_id];

		if (conn->state == HECI_CONN_STATE_UNUSED) {
			idle_conn = conn;
			idle_conn->connection_id = conn_id;
			break;
		}
	}

	if (!idle_conn) {
		LOG_ERR("no free connection");
		resp.status = HECI_CONNECT_STATUS_REJECTED;
		goto out;
	}

	/*
	 * every connection saves its current rx buffer in order to
	 * free it after the client will read the content
	 */
	idle_conn->rx_buffer = heci_get_buffer_from_pool(client);
	if (idle_conn->rx_buffer == NULL) {
		LOG_ERR("no buffer allocated for client %d", client_id);
		resp.status = HECI_CONNECT_STATUS_REJECTED;
		goto out;
	}

	client->n_of_conns++;
	idle_conn->client = client;
	idle_conn->wait_thread_count = 0;
	idle_conn->host_addr = req->host_addr;
	idle_conn->fw_addr = req->fw_addr;
	idle_conn->state = HECI_CONN_STATE_OPEN;
	idle_conn->flow_ctrl_sem = &(flow_ctrl_sems[conn_id]);
	idle_conn->event_cb = client->properties.event_cb;
	idle_conn->param = client->properties.param;

	k_sem_init(idle_conn->flow_ctrl_sem, 0, UINT_MAX);
#ifdef CONFIG_USERSPACE
	for (uint8_t i = 0; i < client->properties.num_of_threads; i++) {
		if (client->properties.thread_handle_list[i] != NULL) {
			k_thread_access_grant(
				client->properties.thread_handle_list[i],
				idle_conn->flow_ctrl_sem);
		}
	}
#endif

	/* send connection handle to client */
	idle_conn->rx_buffer->type = HECI_CONNECT;
	idle_conn->rx_buffer->connection_id = idle_conn->connection_id;
	idle_conn->rx_buffer->length = 0;
	heci_notify_client(idle_conn, HECI_EVENT_NEW_MSG);

	/* send response to host */
	resp.status = HECI_CONNECT_STATUS_SUCCESS;

	LOG_DBG("client connect to host conn=%d(%d<->%d)",
		idle_conn->connection_id, req->host_addr, req->fw_addr);
out:
	heci_send_proto_msg(HECI_DRIVER_ADDRESS, HECI_DRIVER_ADDRESS,
			    true, (uint8_t *)&resp, sizeof(resp));

}

static void heci_disconnect(heci_bus_msg_t *msg)
{
	heci_conn_t *conn;
	heci_disconn_req_t *req = (heci_disconn_req_t *)msg->payload;

	if (msg->hdr.len != sizeof(heci_disconn_req_t)) {
		LOG_ERR("wrong DISCONN_REQ len %d", msg->hdr.len);
		return;
	}

	/* Look-up for a connection in either HECI_CONN_STATE_OPEN state
	 * or HECI_CONN_STATE_CONNECTION_REQUEST state
	 */
	conn = heci_find_conn(req->fw_addr, req->host_addr,
			      HECI_CONN_STATE_OPEN |
			      HECI_CONN_STATE_CONNECTION_REQUEST);
	if (conn != NULL) {
		LOG_DBG("disconnect req from host, conn: %d(%d<->%d)",
			conn->connection_id, req->host_addr, req->fw_addr);

		if (conn->state & HECI_CONN_STATE_DISCONNECTING) {
			/*
			 * The connection is already in process of
			 * disconnecting, no need to signal to client
			 */
			conn->state = HECI_CONN_STATE_DISCONNECTING
				      | HECI_CONN_STATE_SEND_DISCONNECT_RESP;
		} else {
			conn->state = HECI_CONN_STATE_DISCONNECTING
				      | HECI_CONN_STATE_SEND_DISCONNECT_RESP;
			heci_notify_client(conn, HECI_EVENT_DISCONN);
		}
	} else {
		LOG_ERR("invalid disconn req-host_addr = %d fw_addr = %d",
			req->host_addr, req->fw_addr);

		/* send a disconnect response to host */
		heci_disconn_resp_t resp = { 0 };

		resp.command = HECI_BUS_MSG_CLIENT_DISCONNECT_RESP;
		resp.host_addr = req->host_addr;
		resp.fw_addr = req->fw_addr;
		resp.status = HECI_CONNECT_STATUS_CLIENT_NOT_FOUND;

		heci_send_proto_msg(HECI_DRIVER_ADDRESS,
				    HECI_DRIVER_ADDRESS,
				    true, (uint8_t *)&resp, sizeof(resp));
	}
}

static void heci_flow_control_recv(heci_bus_msg_t *msg)
{
	heci_conn_t *conn;
	heci_flow_ctrl_t *flowctrl = (heci_flow_ctrl_t *)msg->payload;

	if (msg->hdr.len != sizeof(heci_flow_ctrl_t)) {
		LOG_ERR("wrong FLOW_CTRL len %d", msg->hdr.len);
		return;
	}

	conn = heci_find_conn(flowctrl->fw_addr, flowctrl->host_addr,
			      HECI_CONN_STATE_OPEN);
	if (conn) {
		if (flowctrl->number_of_packets == 0) {
			conn->host_buffers++;
			heci_wakeup_sender(conn,
					   min(1, conn->wait_thread_count));
		} else {
			conn->host_buffers += flowctrl->number_of_packets;
			heci_wakeup_sender(conn,
					   min(flowctrl->number_of_packets,
					       conn->wait_thread_count));
		}

		LOG_DBG(" conn:%d(%d<->%d)", conn->connection_id,
			conn->host_addr, conn->fw_addr);
	} else {
		LOG_ERR("no valid connection");
	}
}

static void heci_reset_resp(heci_bus_msg_t *msg)
{
	heci_conn_t *conn;
	heci_reset_req_t *req = (heci_reset_req_t *)msg->payload;
	heci_reset_resp_t resp = { 0 };

	LOG_DBG("");

	if (msg->hdr.len != sizeof(heci_reset_req_t)) {
		LOG_ERR("wrong RESET_REQ len %d", msg->hdr.len);
		return;
	}

	conn = heci_find_conn(req->fw_addr, req->host_addr,
			      HECI_CONN_STATE_OPEN);
	if (conn) {
		conn->host_buffers = 0;

		resp.command = HECI_BUS_MSG_RESET_RESP;
		resp.host_addr = req->host_addr;
		resp.fw_addr = req->fw_addr;
		resp.status = HECI_CONNECT_STATUS_SUCCESS;
		heci_send_proto_msg(HECI_DRIVER_ADDRESS,
				    HECI_DRIVER_ADDRESS,
				    true,
				    (uint8_t *)&resp, sizeof(resp));
	}

	/*
	 * Just ignore the message for non-existing connection or
	 * in an inappropriate state
	 */
}


static void heci_add_client_resp(heci_bus_msg_t *msg)
{
	int i;

	LOG_DBG("");
	heci_add_client_resp_t *resp = (heci_add_client_resp_t *)msg->payload;

	if (msg->hdr.len != sizeof(heci_add_client_resp_t)) {
		LOG_ERR("wrong ADD_CLIENT_RESP len %dn", msg->hdr.len);
		return;
	}

	if (resp->status != 0) {
		LOG_ERR("can't activate client %d resp status %d",
			resp->client_addr, resp->status);
		return;
	}

	for (i = 0; i < HECI_MAX_NUM_OF_CLIENTS; i++) {
		if (heci_dev.clients[i].client_addr == resp->client_addr) {
			heci_dev.clients[i].active = 1;
			LOG_DBG("client %d active", resp->client_addr);
			return;
		}
	}

	LOG_DBG("client %d not found", resp->client_addr);
}

static void heci_connection_error(heci_conn_t *conn)
{
	heci_disconn_req_t req;

	/* closing the connection and sending disconnect request to host */
	req.command = HECI_BUS_MSG_CLIENT_DISCONNECT_REQ;
	req.host_addr = conn->host_addr;
	req.fw_addr = conn->fw_addr;
	req.reserved = 0;

	heci_send_proto_msg(HECI_DRIVER_ADDRESS, HECI_DRIVER_ADDRESS,
			    true, (uint8_t *)&req, sizeof(req));
}

static heci_conn_t *heci_find_active_conn(uint8_t fw_addr, uint8_t host_addr)
{
	heci_conn_t *conn;

	conn = heci_find_conn(fw_addr, host_addr,
			      HECI_CONN_STATE_OPEN |
			      HECI_CONN_STATE_PROCESSING_MSG);
	if (!conn) {
		LOG_ERR("did not find conn %u %u", fw_addr, host_addr);
		return NULL;
	}

	/* if it's the first fragment */
	if (!(conn->state & HECI_CONN_STATE_PROCESSING_MSG)) {
		conn->rx_buffer = heci_get_buffer_from_pool(conn->client);
		if (conn->rx_buffer == NULL) {
			LOG_ERR("connection buffer locked");
			return NULL;
		}

		conn->state |= HECI_CONN_STATE_PROCESSING_MSG;
		conn->rx_buffer->length = 0;
		conn->rx_buffer->type = HECI_REQUEST;
		conn->rx_buffer->connection_id = conn->connection_id;
	}

	return conn;
}

static void heci_copy_to_client_buf(heci_conn_t *conn,
				    uint64_t src_addr, int len, bool dma)
{
	heci_rx_msg_t *rxmsg = conn->rx_buffer;
	heci_client_ctrl_t *client = conn->client;
	uint8_t *dst = &rxmsg->buffer[rxmsg->length];

	/* check if bad packet */
	if ((rxmsg->length + len > client->properties.rx_buffer_len) ||
	    (rxmsg->length + len > client->properties.max_msg_size)) {
		LOG_ERR("invalid buffer len: %d curlen: %d",
			rxmsg->length, len);
		heci_connection_error(conn);
		rxmsg->msg_lock = MSG_UNLOCKED;
		conn->state &= ~HECI_CONN_STATE_PROCESSING_MSG;
		return;
	}

	if (!dma) {
		memcpy(dst, (void *)(uintptr_t)src_addr, len);
	} else {
		LOG_WRN("no dma support for host->pse");
		return;
	}
	rxmsg->length += len;

	rxmsg->type = HECI_REQUEST;
	rxmsg->connection_id = conn->connection_id;
}

static void heci_process_bus_message(heci_bus_msg_t *msg)
{
	uint8_t cmd = msg->payload[0];

	switch (cmd) {
	case HECI_BUS_MSG_VERSION_REQ:
		heci_version_resp(msg);
		break;
	case HECI_BUS_MSG_HOST_STOP_REQ:
		heci_stop_resp(msg);
		break;
	case HECI_BUS_MSG_HOST_ENUM_REQ:
		heci_enum_resp(msg);
		break;
	case HECI_BUS_MSG_HOST_CLIENT_PROP_REQ:
		heci_client_prop(msg);
		break;
	case HECI_BUS_MSG_CLIENT_CONNECT_REQ:
		heci_connect_host(msg);
		break;
	case HECI_BUS_MSG_CLIENT_DISCONNECT_REQ:
		heci_disconnect(msg);
		break;
	case HECI_BUS_MSG_FLOW_CONTROL:
		heci_flow_control_recv(msg);
		break;
	case HECI_BUS_MSG_RESET_REQ:
		heci_reset_resp(msg);
		break;
	case HECI_BUS_MSG_ADD_CLIENT_RESP:
		heci_add_client_resp(msg);
		break;
#if CONFIG_HECI_USE_DMA
	case HECI_BUS_MSG_DMA_ALLOC_NOTIFY_REQ:
		heci_dma_alloc_notification(msg);
		break;
	case HECI_BUS_MSG_DMA_XFER_REQ: /* DMA transfer HOST to FW */
		LOG_DBG("host got host dma data req");
		break;
	case HECI_BUS_MSG_DMA_XFER_RESP: /* Ack for DMA transfer from FW */
		LOG_DBG("host got fw dma data");
		heci_dma_xfer_ack(msg);
		break;
#endif
	/* should never get this */
	case HECI_BUS_MSG_CLIENT_DISCONNECT_RESP:
		LOG_ERR("receiving DISCONNECT_RESP message");
	default:
		break;
	}
}

static void heci_process_client_message(heci_bus_msg_t *msg)
{
	heci_conn_t *conn;

	conn = heci_find_active_conn(msg->hdr.fw_addr, msg->hdr.host_addr);
	if (!conn) {
		LOG_ERR(" no valid connection");
		return;
	}

	LOG_DBG(" conn:%d(%d<->%d)", conn->connection_id,
		msg->hdr.host_addr, msg->hdr.fw_addr);
	heci_copy_to_client_buf(conn, (uintptr_t)msg->payload,
				msg->hdr.len, false);

	/* send msg to client */
	if (msg->hdr.last_frag) {
		conn->state &= ~HECI_CONN_STATE_PROCESSING_MSG;
		heci_notify_client(conn, HECI_EVENT_NEW_MSG);
	}
}

void heci_reset(void)
{
	int i;

	k_mutex_lock(&dev_lock, K_FOREVER);

	for (i = 0; i < HECI_MAX_NUM_OF_CONNECTIONS; i++) {
		heci_conn_t *conn = &heci_dev.connections[i];

		if (conn->state & HECI_CONN_STATE_OPEN) {
			heci_connection_reset(conn);
		} else if (conn->state & HECI_CONN_STATE_DISCONNECTING) {
			/* client was already signaled with disconnect event
			 * no need to signal again
			 */
			conn->state = HECI_CONN_STATE_DISCONNECTING;
		}
	}

	k_mutex_unlock(&dev_lock);
}

static bool heci_client_find(heci_client_t *client)
{
	int i;
	heci_client_ctrl_t *client_ctrl = heci_dev.clients;

	for (i = 0; i < HECI_MAX_NUM_OF_CLIENTS; i++, client_ctrl++) {
		if (memcmp(&client->protocol_id,
			   &client_ctrl->properties.protocol_id,
			   sizeof(client->protocol_id)) == 0) {
			return true;
		}
	}

	LOG_INF("HECI client not found");
	return false;
}

static void heci_send_new_client_msg(heci_client_ctrl_t *client)
{
	heci_add_client_req_t req = { 0 };

	if (!heci_dev.notify_new_clients) {
		return;
	}

#define ASSIGN_PROP(ID) (req.client_properties.ID = client->properties.ID)

	req.client_addr = client->client_addr;
	req.command = HECI_BUS_MSG_ADD_CLIENT_REQ;

	ASSIGN_PROP(protocol_id);
	ASSIGN_PROP(dma_enabled);
	ASSIGN_PROP(dma_header_length);
	ASSIGN_PROP(max_msg_size);
	ASSIGN_PROP(max_n_of_connections);
	ASSIGN_PROP(protocol_ver);
	req.client_properties.fixed_address = client->client_addr;

#undef ASSIGN_PROP

	heci_send_proto_msg(HECI_DRIVER_ADDRESS, HECI_DRIVER_ADDRESS,
			    true, (uint8_t *) &req, sizeof(req));
}

int heci_complete_disconnect(uint32_t conn_id)
{
	int ret;
	heci_conn_t *conn;

	if (conn_id >= HECI_MAX_NUM_OF_CONNECTIONS) {
		LOG_ERR("bad conn id %u", conn_id);
		return -EINVAL;
	}

	k_mutex_lock(&dev_lock, K_FOREVER);

	conn = &heci_dev.connections[conn_id];
	if (!(conn->state & HECI_CONN_STATE_DISCONNECTING)) {
		LOG_ERR("disconn conn %u, state 0x%x",
			conn_id, conn->state);
		ret = -EINVAL;
		goto out_unlock;
	}

	LOG_DBG(" conn %u(%d<->%d)",
		conn_id, conn->host_addr, conn->fw_addr);

	/* clean connection rx buffer */
	if (conn->rx_buffer != NULL) {
		heci_rx_msg_t *buf = conn->rx_buffer;

		buf->type = 0;
		buf->length = 0;
		buf->connection_id = 0;
		buf->msg_lock = MSG_UNLOCKED;
	}

	if (conn->state & HECI_CONN_STATE_SEND_DISCONNECT_RESP) {
		/* send a disconnect response to host with the old host_addr */
		heci_disconn_resp_t resp = { 0 };

		resp.command = HECI_BUS_MSG_CLIENT_DISCONNECT_RESP;
		resp.host_addr = conn->host_addr;
		resp.fw_addr = conn->fw_addr;
		resp.status = HECI_CONNECT_STATUS_SUCCESS;

		heci_send_proto_msg(HECI_DRIVER_ADDRESS,
				    HECI_DRIVER_ADDRESS,
				    true,
				    (uint8_t *)&resp, sizeof(resp));
	}

	conn->client->n_of_conns--;
	memset(conn, 0, sizeof(*conn));

out_unlock:
	k_mutex_unlock(&dev_lock);
	return 0;
}

int heci_register(heci_client_t *client)
{
	int i;
	heci_client_ctrl_t *client_ctrl = heci_dev.clients;

	if ((client == NULL) ||
	    (client->rx_msg == NULL) ||
	    (client->rx_msg->buffer == NULL)) {
		LOG_ERR("can't register client for bad params");
		return -EINVAL;
	}

	if (client->max_msg_size > HECI_MAX_MSG_SIZE) {
		LOG_ERR("client msg size couldn't be larger than 4K");
		return -EINVAL;
	}

	k_mutex_lock(&dev_lock, K_FOREVER);

	/* Check if can find the client on the list. */
	if (heci_client_find(client)) {
		LOG_ERR("client already registered");
		k_mutex_unlock(&dev_lock);
		return -EBUSY;
	}

	for (i = 0; i < HECI_MAX_NUM_OF_CLIENTS; i++, client_ctrl++) {
		if (client_ctrl->client_addr == 0) {
			break;
		}
	}

	if (i == HECI_MAX_NUM_OF_CLIENTS) {
		k_mutex_unlock(&dev_lock);
		LOG_ERR("heci client resource is used up, failed to register");
		return -1;
	}
	heci_dev.registered_clients++;
	client_ctrl->properties = *client;
	client_ctrl->client_addr = (uint16_t)(i + HECI_FIXED_CLIENT_NUM);
	client_ctrl->n_of_conns = 0;
	client_ctrl->active = false;
	heci_send_new_client_msg(client_ctrl);
#ifdef CONFIG_USERSPACE
	const struct device *ipc_host =
		device_get_binding("IPC_HOST");

	for (i = 0; i < client->num_of_threads; i++) {
		if (client->thread_handle_list[i] != NULL) {
			k_thread_access_grant(
				client->thread_handle_list[i],
				&dev_lock, ipc_host);
		}
	}
#endif

	k_mutex_unlock(&dev_lock);
	LOG_DBG("client is registered successfully with client id = %d",
		client_ctrl->client_addr);
	return 0;
}

static void heci_process_message(heci_bus_msg_t *msg)
{
	k_mutex_lock(&dev_lock, K_FOREVER);

	switch (msg->hdr.fw_addr) {
	case HECI_DRIVER_ADDRESS:
		heci_process_bus_message(msg);
		break;
	case HECI_SYSTEM_STATE_CLIENT_ADDR:
		LOG_DBG("HECI_SYSTEM_STATE");
		break;
	case HECI_FW_STATE_CLIENT_ADDR:
		LOG_DBG("HECI_FW_STATE");
		break;
	default:
		heci_process_client_message(msg);
	}
	k_mutex_unlock(&dev_lock);
}

static inline void ack_host(const struct device *dev, uint32_t drbl_ack)
{
	ipc_send_ack(dev, drbl_ack, NULL, 0);
#ifdef CONFIG_SYS_MNG
	send_rx_complete(dev);
#endif
}

int ipc_heci_handler(const struct device *dev, uint32_t drbl)
{
	__ASSERT((dev != NULL), "invalid device\n");
	uint8_t buf[HECI_IPC_PACKET_SIZE] = { 0 };
	heci_bus_msg_t *heci_msg = (heci_bus_msg_t *)buf;

	int ret;
	int msg_len = IPC_HEADER_GET_LENGTH(drbl);
	uint32_t drbl_ack = drbl & (~BIT(IPC_DRBL_BUSY_OFFS));

	if (msg_len > IPC_DATA_LEN_MAX) {
		LOG_ERR("invalid IPC msg len");
		ack_host(dev, drbl_ack);
		return -1;
	}

	ret = ipc_read_msg(dev, NULL, (uint8_t *)heci_msg, msg_len);
	ack_host(dev, drbl_ack);
	if (ret) {
		LOG_ERR(" err %d", ret);
		return -1;
	}

	/* judge if it is a valid heci msg*/
	if (heci_msg->hdr.len + sizeof(heci_msg->hdr) == msg_len) {
		DUMP_HECI_MSG(drbl, heci_msg, false, true);
		heci_process_message(heci_msg);
		return 0;
	} else {
		LOG_ERR("invalid HECI msg");
		return -1;
	}
}

static void sx_heci_handler(sedi_pm_sx_event_t event, void *ctx)
{
	if (event == PM_EVENT_HOST_SX_ENTRY) {
		mng_sx_entry();
		sedi_fwst_set(ILUP_HOST, 0);
		heci_reset();
	}
}

int heci_init(struct device *arg)
{
	int ret;

	ARG_UNUSED(arg);

	LOG_DBG("heci started");

	k_mutex_init(&dev_lock);
	ret = host_protocol_register(IPC_PROTOCOL_HECI, ipc_heci_handler);
	if (ret != 0) {
		LOG_ERR("fail to add ipc_heci_handler as cb fun");
	}
	sedi_pm_register_sx_notification(sx_heci_handler, NULL);
	return 0;
}

#if CONFIG_HECI_MSG_DUMP
/*dump heci msg, for debug usage*/
APP_SHARED_VAR uint32_t in_msg_count;
APP_SHARED_VAR uint32_t out_msg_count;
void dump_heci_msg(uint32_t drbl, heci_bus_msg_t *msg,
		   bool only_hdr, bool is_in_msg)
{
	heci_hdr_t *hdr = &msg->hdr;

	if (msg == NULL) {
		return;
	}

	LOG_DBG("---------%s------- index = %ld, header = %lx",
		is_in_msg ? "IN" : "OUT",
		is_in_msg ? (in_msg_count++) : (out_msg_count++),
		*(uint32_t *)hdr);

	if (!only_hdr) {
		LOG_HEXDUMP_DBG((uint8_t *)msg->payload,
				hdr->len, "msg content");
	}
}
#endif

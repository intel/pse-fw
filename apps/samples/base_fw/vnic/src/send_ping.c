/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Helper file for VNIC main application.
 * This contains the following functions:
 * remove_ipv4_ping_handler() -- Unregister IPv4 ping handler
 * handle_ipv4_echo_reply()   -- Handle IPv4 echo reply packet
 * ping_ipv4() -- Send ping request to host
 * send_ping() -- Initiate ping send to host and wait for reply
 * @{
 */

/* Local Includes */
#include <logging/log.h>
#include <random/rand32.h>
LOG_MODULE_REGISTER(ping, LOG_LEVEL_INF);
#include "icmpv4.h"
#include "net_private.h"

/** Semaphore for handling ping response timeout */
K_SEM_DEFINE(timeout, 0, 1);

static enum net_verdict handle_ipv4_echo_reply(struct net_pkt *pkt,
					       struct net_ipv4_hdr *ip_hdr,
					       struct net_icmp_hdr *icp_hdr);

/* Ping handler for Internet Protocol version 4 (IPv4) */
static struct net_icmpv4_handler ping4_handler = {
.type = NET_ICMPV4_ECHO_REPLY, .code = 0, .handler = handle_ipv4_echo_reply,
};

/* @brief remove IPv4 ping handler
 *
 * Unregister IPv4 ping handler
 *
 */
static inline void remove_ipv4_ping_handler(void)
{
	net_icmpv4_unregister_handler(&ping4_handler);
}

/* @brief handle IPv4 echo reply
 *
 * Handle received echo reply packet and
 * unregister ping handler.
 *
 */
static enum net_verdict handle_ipv4_echo_reply(struct net_pkt *pkt,
					       struct net_ipv4_hdr *ip_hdr,
					       struct net_icmp_hdr *icmp_hdr)
{
	LOG_INF("Received echo reply from %s to %s",
		log_strdup(net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src)),
		log_strdup(net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst)));

	k_sem_give(&timeout);
	remove_ipv4_ping_handler();

	net_pkt_unref(pkt);
	return NET_OK;
}

/* @brief send ping request
 *
 * Register IPv4 ping handler and send
 * ping request to host.
 *
 */
static int ping_ipv4(char *host)
{
	struct in_addr ipv4_target;
	int ret;

	/* Convert received string to IP address. */
	if (net_addr_pton(AF_INET, host, &ipv4_target) < 0) {
		return -EINVAL;
	}

	net_icmpv4_register_handler(&ping4_handler);
	ret = net_icmpv4_send_echo_request(
		net_if_ipv4_select_src_iface(&ipv4_target), &ipv4_target,
		sys_rand32_get(), sys_rand32_get(), 0, 0);
	if (ret) {
		LOG_ERR("net_icmpv4_send_echo_request failed");
		remove_ipv4_ping_handler();
	} else {
		LOG_INF("Sent a ping to %s", log_strdup(host));
	}

	return ret;
}

/* @brief send ping to host
 *
 * Initiate ping send to the IP address specified by host string
 * and wait for reply.
 *
 */
int send_ping(char *host)
{
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = ping_ipv4(host);
		if (ret) {
			if (ret == -EIO) {
				LOG_INF("Cannot send IPv4 ping");
			} else if (ret == -EINVAL) {
				LOG_INF("Invalid IP address");
			}

			return -ENOEXEC;
		}
	}

	/* Wait for ping response */
	ret = k_sem_take(&timeout, K_SECONDS(2));
	if (ret == -EAGAIN) {
		LOG_INF("Ping timeout");
		remove_ipv4_ping_handler();
		return -ETIMEDOUT;
	}

	return 0;
}

/**
 * @}
 */

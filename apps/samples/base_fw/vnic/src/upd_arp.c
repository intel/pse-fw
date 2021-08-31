/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Helper file for VNIC main application.
 * This contains the following functions:
 * net_arp_dev_init() -- Initialize ARP context with driver data
 * if_get_addr() -- Get IPv4 address for the interface provided
 * prepare_arp_reply() -- Populate all fields of ARP reply packet
 * prepare_arp_request() -- Populate all fields of ARP request packet
 * @{
 */

/* Local Includes */
#include "logging/log.h"
LOG_MODULE_REGISTER(net_test, CONFIG_NET_ARP_LOG_LEVEL);
#include <device.h>
#include <errno.h>
#include <ethernet/arp.h>
#include <init.h>
#include <linker/sections.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_ip.h>
#include <net/dummy.h>
#include <stddef.h>
#include <string.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include "net_private.h"

/* Handler for application data */
static char *app_data = "0123456789";

/* Pointer to pending packet */
static struct net_pkt *pending_pkt;

/* Ethernet address for the link */
static struct net_eth_addr hwaddr = { { 0x46, 0x81, 0x59, 0xfe, 0x95, 0x04 } };

/* Address Resolution Protocol (ARP) context */
struct net_arp_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

/* @brief ARP device init
 *
 * Initialize ARP context with driver data
 *
 */
int net_arp_dev_init(struct device *dev)
{
	struct net_arp_context *net_arp_context = dev->data;

	net_arp_context = net_arp_context;

	return 0;
}

/* @brief get IPv4 address for given interface
 *
 * Get IPv4 address for the interface provided
 * if a match is found, otherwise return NULL.
 *
 */
static inline struct in_addr *if_get_addr(struct net_if *iface)
{
	int i;

	/* Scan all IPv4 interfaces and return the IP address
	 * if a match is found, otherwise return NULL
	 */
	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (iface->config.ip.ipv4->unicast[i].is_used &&
		    iface->config.ip.ipv4->unicast[i].address.family ==
		    AF_INET &&
		    iface->config.ip.ipv4->unicast[i].addr_state ==
		    NET_ADDR_PREFERRED) {
			return &iface->config.ip.ipv4->unicast[i]
			       .address.in_addr;
		}
	}

	return NULL;
}

/* @brief prepare ARP reply
 *
 * Populate all fields of ARP reply packet for the given address
 * and return the filled packet.
 *
 */
static inline struct net_pkt *prepare_arp_reply(struct net_if *iface,
						struct net_pkt *req,
						struct net_eth_addr *addr,
						struct net_eth_hdr **eth_rep)
{
	struct net_pkt *pkt;
	struct net_arp_hdr *hdr;
	struct net_eth_hdr *eth;

	/* Allocate a network packet and buffer at once. */
	pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct net_eth_hdr) +
					sizeof(struct net_eth_hdr) +
					sizeof(struct net_arp_hdr),
					AF_UNSPEC, 0, K_SECONDS(1));
	__ASSERT(pkt, "out of mem reply");

	eth = NET_ETH_HDR(pkt);


	net_buf_add(pkt->buffer, sizeof(struct net_eth_hdr));

	/* Remove data from the beginning of the buffer. */
	net_buf_pull(pkt->buffer, sizeof(struct net_eth_hdr));

	(void)memset(&eth->dst.addr, 0xff, sizeof(struct net_eth_addr));
	memcpy(&eth->src.addr, net_if_get_link_addr(iface)->addr,
	       sizeof(struct net_eth_addr));
	eth->type = htons(NET_ETH_PTYPE_ARP);

	*eth_rep = eth;

	net_buf_add(pkt->buffer, sizeof(struct net_eth_hdr));
	net_buf_pull(pkt->buffer, sizeof(struct net_eth_hdr));

	hdr = NET_ARP_HDR(pkt);

	/* Populate all fields of ARP reply packet. */
	hdr->hwtype = htons(NET_ARP_HTYPE_ETH);
	hdr->protocol = htons(NET_ETH_PTYPE_IP);
	hdr->hwlen = sizeof(struct net_eth_addr);
	hdr->protolen = sizeof(struct in_addr);
	hdr->opcode = htons(NET_ARP_REPLY);

	memcpy(&hdr->dst_hwaddr.addr, &eth->src.addr,
	       sizeof(struct net_eth_addr));
	memcpy(&hdr->src_hwaddr.addr, addr, sizeof(struct net_eth_addr));

	/* Copy source and destination IPv4 addresses. */
	net_ipaddr_copy(&hdr->dst_ipaddr, &NET_ARP_HDR(req)->src_ipaddr);
	net_ipaddr_copy(&hdr->src_ipaddr, &NET_ARP_HDR(req)->dst_ipaddr);

	/* Increment data length of buffer to account for more data at end. */
	net_buf_add(pkt->buffer, sizeof(struct net_arp_hdr));

	return pkt;
}

/* @brief prepare ARP request
 *
 * Populate all fields of ARP request packet for the given address
 * and return the filled packet.
 *
 */
static inline struct net_pkt *prepare_arp_request(struct net_if *iface,
						  struct net_pkt *req,
						  struct net_eth_addr *addr,
						  struct net_eth_hdr **eth_hdr)
{
	struct net_pkt *pkt;
	struct net_arp_hdr *hdr, *req_hdr;
	struct net_eth_hdr *eth, *eth_req;

	/* Allocate a network packet and buffer at once. */
	pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct net_eth_hdr) +
					sizeof(struct net_arp_hdr),
					AF_UNSPEC, 0, K_SECONDS(1));
	__ASSERT(pkt, "out of mem request");

	eth_req = NET_ETH_HDR(req);
	eth = NET_ETH_HDR(pkt);

	/* Remove data from the beginning of the buffer. */
	net_buf_pull(req->buffer, sizeof(struct net_eth_hdr));

	req_hdr = NET_ARP_HDR(req);

	(void)memset(&eth->dst.addr, 0xff, sizeof(struct net_eth_addr));
	memcpy(&eth->src.addr, addr, sizeof(struct net_eth_addr));

	eth->type = htons(NET_ETH_PTYPE_ARP);
	*eth_hdr = eth;

	net_buf_pull(pkt->buffer, sizeof(struct net_eth_hdr));

	hdr = NET_ARP_HDR(pkt);

	/* Populate all fields of ARP request packet. */
	hdr->hwtype = htons(NET_ARP_HTYPE_ETH);
	hdr->protocol = htons(NET_ETH_PTYPE_IP);
	hdr->hwlen = sizeof(struct net_eth_addr);
	hdr->protolen = sizeof(struct in_addr);
	hdr->opcode = htons(NET_ARP_REQUEST);

	(void)memset(&hdr->dst_hwaddr.addr, 0x00, sizeof(struct net_eth_addr));
	memcpy(&hdr->src_hwaddr.addr, addr, sizeof(struct net_eth_addr));

	/* Copy source and destination IPv4 addresses. */
	net_ipaddr_copy(&hdr->src_ipaddr, &req_hdr->src_ipaddr);
	net_ipaddr_copy(&hdr->dst_ipaddr, &req_hdr->dst_ipaddr);

	/* Increment data length of buffer to account for more data at end. */
	net_buf_add(pkt->buffer, sizeof(struct net_arp_hdr));

	return pkt;
}

struct net_arp_context net_arp_context_data;

/* @brief update_arp_chache
 *
 * Updating the arp chache.
 *
 */
void update_arp_cache(void)
{

	struct net_eth_hdr *eth_hdr = NULL;
	struct net_pkt *pkt, *pkt2;
	struct net_if *iface;
	struct net_if_addr *ifaddr;
	struct net_arp_hdr *arp_hdr;
	struct net_ipv4_hdr *ipv4;
	int len;

	struct in_addr dst = { { { 192, 168, 0, 2 } } };
	struct in_addr dst_far = { { { 10, 11, 12, 13 } } };
	struct in_addr dst_far2 = { { { 172, 16, 14, 186 } } };
	struct in_addr src = { { { 192, 168, 0, 1 } } };
	struct in_addr netmask = { { { 255, 255, 255, 0 } } };
	struct in_addr gw = { { { 192, 168, 0, 42 } } };

	net_arp_init();

	iface = net_if_get_default();

	net_if_ipv4_set_gw(iface, &gw);
	net_if_ipv4_set_netmask(iface, &netmask);
	/* Unicast test */
	ifaddr = net_if_ipv4_addr_add(iface, &src, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		printk("Cannot add address");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	len = strlen(app_data);

	/* Application data for testing */
	pkt = net_pkt_alloc_with_buffer(iface,
					sizeof(struct net_ipv4_hdr) + len,
					AF_INET,
					0,
					K_SECONDS(1));
	net_pkt_lladdr_src(pkt)->addr = (uint8_t *)net_if_get_link_addr(iface);
	net_pkt_lladdr_src(pkt)->len = sizeof(struct net_eth_addr);


	ipv4 = (struct net_ipv4_hdr *)net_buf_add(pkt->buffer,
						  sizeof(struct net_ipv4_hdr));
	net_ipaddr_copy(&ipv4->src, &src);
	net_ipaddr_copy(&ipv4->dst, &dst);

	memcpy(net_buf_add(pkt->buffer, len), app_data, len);

	pkt2 = net_arp_prepare(pkt, &NET_IPV4_HDR(pkt)->dst, NULL);

	/* The ARP cache should now have a link to pending net_pkt
	 * that is to be sent after we have got an ARP reply.
	 */
	pending_pkt = pkt;

	/* We could have send the new ARP request but for this test we
	 * just free it.
	 */
	net_pkt_unref(pkt2);

	/* Then a case where target is not in the same subnet */
	net_ipaddr_copy(&ipv4->dst, &dst_far);
	pkt2 = net_arp_prepare(pkt, &NET_IPV4_HDR(pkt)->dst, NULL);

	arp_hdr = NET_ARP_HDR(pkt2);
	net_pkt_unref(pkt2);

	/* Try to find the same destination again, this should fail as there
	 * is a pending request in ARP cache.
	 */
	net_ipaddr_copy(&ipv4->dst, &dst_far);

	/* Make sure prepare will not free the pkt because it will be
	 * needed in the later test case.
	 */
	net_pkt_ref(pkt);

	pkt2 = net_arp_prepare(pkt, &NET_IPV4_HDR(pkt)->dst, NULL);

	net_pkt_unref(pkt2);

	/* Try to find the different destination, this should fail too
	 * as the cache table should be full.
	 */
	net_ipaddr_copy(&ipv4->dst, &dst_far2);

	/* Make sure prepare will not free the pkt because it will be
	 * needed in the next test case.
	 */
	net_pkt_ref(pkt);

	pkt2 = net_arp_prepare(pkt, &NET_IPV4_HDR(pkt)->dst, NULL);

	/* Restore the original address so that following test case can
	 * work properly.
	 */
	net_ipaddr_copy(&ipv4->dst, &dst);

	/* The arp request packet is now verified, create an arp reply.
	 * The previous value of pkt is stored in arp table and is not lost.
	 */
	pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct net_eth_hdr) +
					sizeof(struct net_arp_hdr),
					AF_UNSPEC, 0, K_SECONDS(1));

	arp_hdr = NET_ARP_HDR(pkt);
	net_buf_add(pkt->buffer, sizeof(struct net_arp_hdr));
	net_ipaddr_copy(&arp_hdr->dst_ipaddr, &dst);
	net_ipaddr_copy(&arp_hdr->src_ipaddr, &src);

	pkt2 = prepare_arp_reply(iface, pkt, &hwaddr, &eth_hdr);
	/* The pending packet should now be sent */
	switch (net_arp_input(pkt2, eth_hdr)) {
	case NET_OK:
	case NET_CONTINUE:
		break;
	case NET_DROP:
		break;
	}

	/* Yielding so that network interface TX thread can proceed. */
	k_yield();

	net_pkt_unref(pkt);
}

/**
 * @}
 */

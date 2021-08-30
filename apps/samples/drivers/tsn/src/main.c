/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @file
 * @brief Sample Application for TSN
 * This sample mainly demonstrate how to exercise Time-Based Scheduling(TBS)
 * or TXTIME feature by just sending the embedded TXTIME UDP packets from the
 * user space thread. Please refer to section 3.13 in “IntelPSE_SDK_User_Guide”
 * for more details.
 * @{
 */

/**
 * @brief How to Build sample application.
 * Please refer to “IntelPSE_SDK_Get_Started_Guide” for more details on how to
 * build the sample codes.
 */

/**
 * @brief Hardware setup.
 * Please refer to section 3.13.1 in “IntelPSE_SDK_User_Guide” for more details
 * of TSN hardware setup.
 */

#define LOG_MODULE_NAME tsn_sample_app
#define LOG_LEVEL CONFIG_TSN_SAMPLE_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* Local Includes */
#include <ksched.h>
#include <sys/fdtable.h>
#include <sys/printk.h>
#include <sys/libc-hooks.h>
#include <net/net_if.h>
#include <net/ethernet_mgmt.h>
#include <net/socket.h>
#include <net/gptp.h>
#include <ptp_clock.h>
#include <stdlib.h>
#include <stdio.h>
#include <zephyr.h>

/** Packet send string */
#define SEND_STR "TBS frame no: "
/** Packet send string length */
#define SEND_STR_SZ (sizeof(SEND_STR) - 1)
/** Packet numbering string length */
#define SEND_STR_COUNTER_SZ 5
/** Macro define for converting second to nanosecond */
#define ONE_SEC_IN_NANOSEC 1000000000ULL
/** UDP packet payload maximum length */
#define MAX_UDP_PAYLOAD_SZ 1472
/** Synopsys EQoS controller defined maximum TBS interval */
#define DWC_EQOS_MAX_TXTIME_INTERVAL 128000000
/** Zephyr network stack buffer full wait polling interval */
#define BUSY_WAIT_USEC 200
/** Thread stack size macro define */
#define STACKSIZE 4096
/** Thread resource pool heap memory size */
/* The Heap memory size is the aligned total count of below function parameters
 * [used in bind] 2*sizeof(struct sockaddr_in)
 * [used in net_addr_pton] 2*sizeof(CONFIG_TSN_SAMPLE_UDP_SEND_TARGET_IPV4_ADDR)
 * [used in net_addr_pton] 2*sizeof(struct in_addr)
 * [used in setsockopt] 2*sizeof(unsigned char), 2*sizeof(bool)
 * [used in sendmsg] 2*sizeof(CONFIG_TSN_SAMPLE_UDP_SEND_DATA_LEN)
 */
#define MEM_POOL_SZ 3072

/** Zephyr thread structure and configuration */
static struct k_thread txtime_test_thread;
static K_THREAD_STACK_DEFINE(txtime_test_stack, STACKSIZE);
#if defined(CONFIG_TSN_SAMPLE_USER_MODE)
#if Z_LIBC_PARTITION_EXISTS
static struct k_mem_domain txtime_test_mem_dom;
#endif
K_HEAP_DEFINE(txtime_test_pool, MEM_POOL_SZ);
#endif

/** gPTP callback structure */
static struct gptp_phase_dis_cb phase_dis;
/** Time variables used for active sync status checking */
static struct net_ptp_time old_sync_time, new_sync_time;
/** Grant Master present status variable */
static bool gm_present;

/**
 * @brief TXTIME/TBS send function
 * Configure the socket API to send UDP packet together with TXTIME data which
 * is calculated from the obtained PTP clock and build config setting.
 * Iterate until the total amount in build config setting has reached.
 */
int txtime_send(const struct device *clk, struct in_addr src_addr,
		bool do_txtime)
{
	int sock, i, ret, offset, delay, interval = 0, len = 0;
	union {
		struct cmsghdr hdr;
		unsigned char  buf[CMSG_SPACE(sizeof(uint64_t))];
	} cmsgbuf;
	char buf[MAX_UDP_PAYLOAD_SZ] = SEND_STR;
	struct sockaddr_in bind_addr, client_addr;
	struct net_ptp_time time = {
		.second = 0,
		.nanosecond = 0,
	};
	unsigned char priority = 0;
	struct iovec io_vector[1];
	struct cmsghdr *cmsg;
	struct msghdr msg;
	uint64_t txtime = 0;

	/* Create a UDP socket */
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	/* Bind the socket to the net interface */
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr = src_addr;
	/* Random source port number */
	bind_addr.sin_port = htons(0);
	bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr));

	/* Fill up the destination IPv4 address */
	if (net_addr_pton(AF_INET, CONFIG_TSN_SAMPLE_UDP_SEND_TARGET_IPV4_ADDR,
	    &client_addr.sin_addr) < 0) {
		LOG_ERR("Invalid destination address");
		return -EINVAL;
	}

	/* Fill up the destination port for UDP */
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(CONFIG_TSN_SAMPLE_UDP_SEND_TARGET_PORT);

	/* Socket Priority range 0 to 7 */
	priority = CONFIG_TSN_SAMPLE_UDP_SEND_SOCKET_PRIORITY;
	/* Set the socket priority */
	setsockopt(sock, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority));

	/* Initialize the msg structure for sendmsg */
	io_vector[0].iov_base = buf;
	io_vector[0].iov_len = CONFIG_TSN_SAMPLE_UDP_SEND_DATA_LEN;
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = io_vector;
	msg.msg_iovlen = 1;
	msg.msg_name = &client_addr;
	msg.msg_namelen = sizeof(client_addr);

	if (do_txtime && IS_ENABLED(CONFIG_NET_CONTEXT_TXTIME)) {
		bool optval;

		/* Obtain the current PTP clock time */
		ptp_clock_get(clk, &time);

		/* The launch time offset alignment from second */
		offset = CONFIG_TSN_SAMPLE_TXTIME_OFFSET;

		/* The 1st packet send delay */
		delay = CONFIG_TSN_SAMPLE_TXTIME_1ST_DELAY;

		/* The time interval between packets */
		interval = CONFIG_TSN_SAMPLE_TXTIME_INTERVAL;

		/* Calculate the 1st TXTIME */
		txtime = (time.second + delay) << 32 | offset;

		/* Setup cmsg for TXTIME */
		msg.msg_control = &cmsgbuf.buf;
		msg.msg_controllen = sizeof(cmsgbuf.buf);
		cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_len = CMSG_LEN(sizeof(txtime));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_TXTIME;
		*(uint64_t *)CMSG_DATA(cmsg) = txtime;

		/* Enable the socket with TXTIME capability */
		optval = true;
		setsockopt(sock, SOL_SOCKET, SO_TXTIME, &optval,
			   sizeof(optval));
	} else if (do_txtime && !IS_ENABLED(CONFIG_NET_CONTEXT_TXTIME)) {
		LOG_WRN("TXTIME build config has not turn on");
	}

	for (i = 0; i < CONFIG_TSN_SAMPLE_UDP_SEND_PACKETS_COUNT; i++) {
		int retry;

		/* Add frame number into packet before the send */
		snprintf(&buf[SEND_STR_SZ], SEND_STR_COUNTER_SZ, "%04d", i);

		/* Send Packet */
		retry = DWC_EQOS_MAX_TXTIME_INTERVAL / BUSY_WAIT_USEC;
		LOG_DBG("Send packet txtime = %u.%u", (uint32_t)(txtime >> 32),
			(uint32_t)txtime);
		while (((ret = sendmsg(sock, &msg, 0)) < 0) &&
		       (errno == ENOMEM)) {
			k_busy_wait(BUSY_WAIT_USEC);  /* 200us */

			if (!(retry--)) {
				ret = -ETIMEDOUT;
				break;
			}
		}
		/* Terminate if encounter send error */
		if (ret < 0) {
			break;
		} else {
			len += ret;
		}

		/* Re-calculate the TXTIME for next frame */
		if (do_txtime && IS_ENABLED(CONFIG_NET_CONTEXT_TXTIME)) {
			txtime += ((interval * 1000 / ONE_SEC_IN_NANOSEC) << 32
				   | (interval * 1000 % ONE_SEC_IN_NANOSEC));

			txtime = ((uint32_t)txtime % ONE_SEC_IN_NANOSEC) |
				  (((txtime >> 32) + ((uint32_t)txtime /
				   ONE_SEC_IN_NANOSEC)) << 32);

			*(uint64_t *)CMSG_DATA(cmsg) = txtime;
		}
	}

	LOG_INF("Total send bytes: %d", len);

	close(sock);
	return ret < 0 ? ret : 0;
}

/**
 * @brief TXTIME/TBS thread function
 * A wrapper thread function to call txtime_send().
 */
void txtime_test(void *p1, void *p2, void *p3)
{
	struct in_addr addr = { .s_addr = POINTER_TO_UINT(p2) };
	bool txtime_en = !!POINTER_TO_UINT(p3);
	const struct device *clk = p1;
	int ret;

	ret = txtime_send(clk, addr, txtime_en);
	if (ret) {
		LOG_ERR("TXTIME send error: %d", ret);
	}
}

/**
 * @brief TXTIME configuration check function
 * Check if any TX queues has enabled TXTIME feature.
 */
static bool is_txtime_enabled(struct net_if *iface)
{
	struct ethernet_req_params params = { 0 };
	int i;

	params.txtime_param.type = ETHERNET_TXTIME_PARAM_TYPE_ENABLE_QUEUES;
	for (i = 0; i < CONFIG_NET_TC_TX_COUNT; i++) {
		params.txtime_param.queue_id = i;

		if (net_mgmt(NET_REQUEST_ETHERNET_GET_TXTIME_PARAM,
			     iface, &params, sizeof(params))) {
			continue;
		}

		if (params.txtime_param.enable_txtime) {
			return true;
		}
	}

	return false;
}

#ifdef CONFIG_NET_GPTP
/**
 * @brief gPTP time sync callback function
 * A callback implementation to check time sync status from master host
 */
static void gptp_phase_dis_cb(uint8_t *gm_identity,
			      uint16_t *time_base,
			      struct gptp_scaled_ns *last_gm_ph_change,
			      double *last_gm_freq_change)
{
	struct net_ptp_time slave_time;

	/* Update the time sync clock when Grant Master clock is available */
	if (!gptp_event_capture(&slave_time, &gm_present)) {
		if (gm_present) {
			new_sync_time = slave_time;
		}
	}
}
#endif  /* CONFIG_NET_GPTP */

/**
 * @brief TSN sample application main function
 * Prepare the work for thread run, create and start the thread, and
 * iterate the thread works again when the previous thread has ended.
 */
void main(void)
{
	#define BIND_INTERFACE CONFIG_TSN_SAMPLE_BIND_NETIF
	const struct device *dev = device_get_binding(BIND_INTERFACE);
	struct net_if *iface = net_if_lookup_by_dev(dev);
	int if_idx = net_if_get_by_iface(iface);
	uint32_t thread_opt = K_INHERIT_PERMS;
	const struct device *clk = NULL;
	bool txtime_queues_enabled = 0;
	struct in_addr src_addr, *addr;

	LOG_INF("PSE TSN Sample Application");

	/* Check for UDP build config */
	if (!IS_ENABLED(CONFIG_NET_UDP)) {
		LOG_ERR("UDP Build Config option has not been enabled");
		return;
	}

	/* User mode select configuration */
	if (IS_ENABLED(CONFIG_TSN_SAMPLE_USER_MODE)) {
		thread_opt |= K_USER;
	}

	/* Obtain IPv4 address from network interface */
	addr = net_if_ipv4_get_ll(iface, NET_ADDR_PREFERRED);
	if (!addr) {
		addr = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
	}
	/* Set src_addr with the obtained IPv4 address else use INADDR_ANY */
	if (addr) {
		src_addr = *addr;
	} else {
		src_addr.s_addr = htonl(INADDR_ANY);
	}

	/* Obtain the PTP clock handler when TXTIME/TBS is enabled */
	txtime_queues_enabled = is_txtime_enabled(iface);
	if (txtime_queues_enabled) {
		clk = net_eth_get_ptp_clock_by_index(if_idx);
		if (!clk) {
			LOG_ERR("Interface[%d] no PTP support", if_idx);
			return;
		}
	}

	LOG_DBG("Network interface[%d] IPv4 address: %08X; txtime enabled: %s",
		if_idx, ntohl(src_addr.s_addr), txtime_queues_enabled ? "true" :
		"false");

#ifdef CONFIG_NET_GPTP
	/* Register a gPTP callback to obtain new time sync timing */
	gptp_register_phase_dis_cb(&phase_dis, gptp_phase_dis_cb);
#else  /* !CONFIG_NET_GPTP */
	/* To create misalignment when gPTP time sync is not enabled */
	new_sync_time.second = 1;
#endif  /* CONFIG_NET_GPTP */

	while (1) {
		/* Ensure txtime_test only be triggered if the thread has ended
		 * or not started. Besides, if TXTIME has enabled then
		 * txtime_test will start after time sync happened.
		 */
		if (!net_if_is_up(iface) ||
		    (txtime_test_thread.base.thread_state &&
		    !z_is_thread_prevented_from_running(&txtime_test_thread)) ||
		    ((old_sync_time.nanosecond == new_sync_time.nanosecond) &&
		    (old_sync_time.second == new_sync_time.second) &&
		    txtime_queues_enabled)) {
			k_sleep(K_SECONDS(1));
			continue;
		}

#ifdef CONFIG_NET_GPTP
		old_sync_time = new_sync_time;
#endif
		/* Spawn a thread for TXTIME application */
		k_thread_create(&txtime_test_thread, txtime_test_stack,
				STACKSIZE, txtime_test, (void *)clk,
				UINT_TO_POINTER(src_addr.s_addr),
				UINT_TO_POINTER(txtime_queues_enabled),
				CONFIG_TSN_SAMPLE_THREAD_PRIORITY,
				thread_opt, K_FOREVER);
#if defined(CONFIG_TSN_SAMPLE_USER_MODE)
#if Z_LIBC_PARTITION_EXISTS
		k_mem_domain_init(&txtime_test_mem_dom, 0, NULL);
		/* Memory partition for C library globals, stack canary storage
		 * and etc
		 */
		k_mem_domain_add_partition(&txtime_test_mem_dom,
					   &z_libc_partition);
		k_mem_domain_add_thread(&txtime_test_mem_dom,
					&txtime_test_thread);
#endif  /* Z_LIBC_PARTITION_EXISTS */
		/* Assign resource pool for user thread to call socket APIs */
		k_thread_heap_assign(&txtime_test_thread, &txtime_test_pool);
		/* Allow the user thread to access PTP clock */
		k_object_access_grant(clk, &txtime_test_thread);
#endif  /* CONFIG_TSN_SAMPLE_USER_MODE */
		/* Start executing the created thread */
		k_thread_start(&txtime_test_thread);
		/* Sleep for next iteration */
		k_sleep(K_SECONDS(CONFIG_TSN_SAMPLE_THREAD_SLEEP_INTERVAL));
	}

	/* End the created thread */
	k_thread_abort(&txtime_test_thread);

	return;
}

/**
 * @}
 */

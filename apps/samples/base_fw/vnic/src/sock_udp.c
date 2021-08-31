/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Helper file for VNIC main application.
 * This contains the following functions:
 * prepare_sock_v4() -- Create a socket and populate it
 * send_to_host() -- Send data to host over udp
 * communication_over_udp_socket() -- Create a task for sending data to host
 * @{
 */

/* Local Includes */
#include <net/socket.h>
#include <stdio.h>

/* Data to be sent to host over udp socket */
#define TEST_STR "HELLO FROM PSE"

/** Destination port for socket */
#define DST_PORT 4242

/** IP address of the linux host for communication */
#define HOST_IP_ADDRESS "192.168.0.2"

/** Stack size of sender task */
#define STACKSIZE (2048)

/** Priority of udp sender task */
#define UDP_TASK_PRI (12)

/* Sender task stack */
K_THREAD_STACK_DEFINE(sender_stack, STACKSIZE);

/* @brief prepare_sock_v4
 *
 * Create a socket and populate it destination IP address and port number .
 *
 */
static void prepare_sock_v4(const char *addr, uint16_t port, int *sock,
			    struct sockaddr_in *sockaddr)
{
	int rv;

	__ASSERT(addr, "null addr");
	__ASSERT(sock, "null sock");
	__ASSERT(sockaddr, "null sockaddr");

	*sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	__ASSERT(*sock >= 0, "socket open failed");

	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = htons(port);
	rv = inet_pton(AF_INET, addr, &sockaddr->sin_addr);
	__ASSERT(rv == 1, "inet_pton failed");
}

/* @brief send_to_host
 *
 * The function is entry point for sender task, it sends data over udp and
 * aborts task after sending data.
 *
 */
void send_to_host(void *p1, void *p2, void *p3)
{
	ssize_t sent;
	int dst_sock;
	struct sockaddr_in dst_addr;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	prepare_sock_v4(HOST_IP_ADDRESS, DST_PORT, &dst_sock, &dst_addr);

	printk("Sending data to the destination\n");
	sent = sendto(dst_sock, TEST_STR, strlen(TEST_STR), 0,
		      (struct sockaddr *)&dst_addr, sizeof(dst_addr));
	printk("Sent %d bytes\n", sent);
	if (sent != strlen(TEST_STR)) {
		printk("sendto failed\n");
		goto error;
	}
	k_sleep(K_MSEC(1000));

error:
	close(dst_sock);
	k_thread_abort(k_current_get());
}

static struct k_thread sender_thread_info;

/* @breif communication_over_udp_socket
 *
 * This function creates a task for sending data to the host.
 *
 */
void communication_over_udp_socket(void)
{
	/* Thread id of sender task */
	k_tid_t sender_tid;

	/* Cerate sender task */
	sender_tid =
	    k_thread_create(&sender_thread_info, sender_stack, STACKSIZE,
			    send_to_host, NULL, NULL, NULL, UDP_TASK_PRI, 0,
			    K_MSEC(0));
	if (sender_tid != 0) {
		printk("Thread created by sender =%x\n",
		       (unsigned int)sender_tid);
	} else {
		printk("Thread created by sender failed\n");
	}
}

/**
 * @}
 */

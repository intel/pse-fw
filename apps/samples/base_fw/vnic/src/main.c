/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 *  @file
 *  @brief Sample Application for VNIC driver.
 *
 *  This example demonstrates sending data over UDP using the VNIC interface.
 *  The VNIC interface  uses HECI/IPC driver for communcation between PSE and
 *  Host. The application repeatedly sends a pings command to  host.
 *  On successfully getting a response to ping, the application sends a simple
 *  message over UDP to the Linux host.
 *  @{
 */


#include <device.h>
#include <errno.h>
#include <init.h>
#include <net/dummy.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <linker/sections.h>
#include <random/rand32.h>
#include <stddef.h>
#include <string.h>
#include <zephyr.h>

/** Time after which udp application starts after success of ping */
#define APP_TIMEOUT_SEC 10

#define K_SLEEP_DURATION_MS K_MSEC(1000)

/** IP address of the linux host for communication */
#define LINUX_HOST_ADDR "192.168.0.2"



/* @brief update_arp_cache
 *
 * Update the arp table to add hostside IP address and  MAC.
 *
 */
extern void update_arp_cache(void);

/* @brief communication_over_udp_socket
 *
 * The function sends data over udp socket to the host.
 *
 */
extern void communication_over_udp_socket(void);

/* @brief send_ping
 *
 * Send ping to the host.
 *
 */
extern int send_ping(char *ip);


/* @brief main
 *
 * Runs the sample VNIC application
 *
 * Demonstrates communication over the virtual network interface in this
 * application. This applicaton sends message to host after a successful
 * ping.
 *
 */

void main(void)
{

	/* Update the arp cache to add the IP and mac id of host */
	update_arp_cache();
	printk("PSE VNIC Test Application\n");

	/* Keep sending ping request to host till  a successful ping response
	 * is received from the host. For ping to succeed, host side
	 * vnic driver must be installed and  assigned the same IP address as
	 * LINUX_HOST_ADDR and mac address.
	 */
	while (send_ping(LINUX_HOST_ADDR)) {
		k_sleep(K_SLEEP_DURATION_MS);
	}

	/* After a successful ping response from host, send data from the
	 * application in  APP_TIMEOUT_SEC seconds. A corresponding udp receiver
	 * application is required to be started on the host for receiving the
	 * data sent by this application
	 */
	for (int i = 0; i < APP_TIMEOUT_SEC; i++) {
		printk("Starting socket app in %d seconds..\n",
		       (APP_TIMEOUT_SEC - i));
		k_sleep(K_SLEEP_DURATION_MS);
	}

	/* Send data over udp socket. */
	communication_over_udp_socket();
}

/*
 * @}
 */

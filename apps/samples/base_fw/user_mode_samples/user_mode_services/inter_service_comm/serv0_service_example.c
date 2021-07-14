/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample Application for services in User Mode.
 * This example demonstrates how to create and use services
 * in User Mode.
 * @{
 */

/**
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more details how to
 * build the sample codes.
 */

/**
 * @brief Hardware setup.
 * There is no specific external hardware setup required for the
 * User Mode sample application.
 */

/* Local Includes */
#include "common.h"
#include <kernel_structs.h>
#include <sys/printk.h>
#include <string.h>
#include <user_app_framework/user_app_framework.h>
#include <zephyr.h>

static void service_main(void *p1, void *p2, void *p3);

/** Declare kernel objects for sub tasks */
__kernel static struct k_thread service_task1_handle;
__kernel static struct k_thread service_task2_handle;
static K_THREAD_STACK_DEFINE(service_task1_stack, 1024);
static K_THREAD_STACK_DEFINE(service_task2_stack, 1024);
K_FIFO_DEFINE(g_serv0_fifo);

/** Add the requried local kernel object pointer into list. */
static const void *obj_list[] = { &g_serv0_fifo, &service_task1_handle,
				  &service_task2_handle, &service_task1_stack,
				  &service_task2_stack };

/** Declare a global buffer/variable which need to share accross
 * multiple Apps.
 */
APP_SHARED_VAR static struct data_item_t fifo_tx_buff;

/** Define a service which will execute immediately after post kernel
 * init but before Apps main.
 */
DEFINE_USER_MODE_APP(4, USER_MODE_SERVICE | K_PART_SHARED, service_main, 1024,
		     (void **)obj_list, 5, NULL);

/* Sub Tasks which can access resource allocated to this service(0). */
static void service_sub_task1(void *q1, void *q2, void *q3)
{
	/* Use ARG_UNUSED([arg]) to avoid compiler warnings about
	 * unused arguments.
	 */
	ARG_UNUSED(q1);
	ARG_UNUSED(q2);
	ARG_UNUSED(q3);

	printk("Main Service 4 Task 0\n");

	strcpy(fifo_tx_buff.buff, "hello this message from task 0 Service 0\n");

	/* For user thread you cannot use k_fifo_put() and need to use
	 * k_fifo_alloc_put() instead.
	 */
	k_fifo_alloc_put(&g_serv0_fifo, &fifo_tx_buff);

	printk("Main Service 4 Task 0 done\n");
}

static void service_sub_task2(void *p1, void *p2, void *p3)
{
	struct data_item_t *fifo_rx_buff;

	/* Use ARG_UNUSED([arg]) to avoid compiler warnings about
	 * unused arguments.
	 */
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Main Service 4 Task 1\n");

	fifo_rx_buff = k_fifo_get(&g_serv0_fifo, K_FOREVER);
	printk("%s\n", fifo_rx_buff->buff);

	printk("Main Service 4 Task 1 done\n");
}

/* @brief service_main function
 * Service Main Entry.
 * Note: The service main thread will be invoked in
 * cooperative mode and service main entry will be invoke even
 * before main function Service main should complete as quickly
 * as possible and offload all its time consuming work to a sub
 * task to improve boot latency
 */
static void service_main(void *p1, void *p2, void *p3)
{
	k_tid_t thread_a, thread_b;

	/* Use ARG_UNUSED([arg]) to avoid compiler warnings about
	 * unused arguments.
	 */
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Service 4 Main\n");

	thread_a = k_thread_create(&service_task1_handle, service_task1_stack,
				   1024, service_sub_task1, NULL, NULL, NULL, 10,
				   APP_USER_MODE_TASK | K_INHERIT_PERMS, K_FOREVER);
	thread_b = k_thread_create(&service_task2_handle, service_task2_stack,
				   1024, service_sub_task2, NULL, NULL, NULL, 12,
				   APP_USER_MODE_TASK | K_INHERIT_PERMS, K_FOREVER);

	k_thread_start(&service_task1_handle);
	k_thread_start(&service_task2_handle);

	printk("Service 4 Main return\n");
}

/**
 * @}
 */

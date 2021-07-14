/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @file
 * @brief Sample Application for simple user mode application.
 * This example demonstrates how to declare and use global
 * variables in User Mode.
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
#include <kernel_structs.h>
#include <sys/printk.h>
#include <string.h>
#include <user_app_framework/user_app_framework.h>
#include <zephyr.h>

/** Declare a global varible which only this App(6) can access. */
APP_GLOBAL_VAR(6) static char g_cnt;

/* @brief main function
 * Access and prints the value of global variable g_cnt.
 */
static void app_main(void *p1, void *p2, void *p3)
{
	/* Use ARG_UNUSED([arg]) to avoid compiler warnings about
	 * unused arguments.
	 */
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("App 6, g_cnt:%d\n", ++g_cnt);
}

/* Define a Simple App which does not require any kernel object access. */
DEFINE_USER_MODE_APP(6, USER_MODE_APP | K_PART_GLOBAL, app_main, 1024, NULL, 0,
		     NULL);

/**
 * @}
 */

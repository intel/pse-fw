/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample Application for ECLite(Embedded Controller Lite)
 * and OOB(Out-of-Band Manageability) service.
 * This example demonstrates how to enable ECLite and
 * OOB service.
 * @{
 */

/**
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more details on how to
 * build the sample codes.
 */

/**
 * @brief Hardware setup.
 * Please refer to the Intel(R) PSE SDK User Guide for more details
 * regarding hardware setup.
 */

/* Local Includes */
#include <fw_version.h>
#include <logging/log.h>
#include <sys/printk.h>
#include <sedi.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <stdlib.h>
#include <user_app_framework/user_app_framework.h>
#include <version.h>
#include <zephyr.h>

/* @brief firmware version
 * Display PSE firmware version
 */
static int cmd_fw_version(const struct shell *shell, size_t argc, char **argv)
{

	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("\nFW VERSION: %s.%s.%s\n", FWVER_MAJOR, FWVER_MINOR,
	       FWVER_PATCH);
	printk("FW HW IP: %s\n", FWVER_HW_IP);
	printk("FW BUILD: DATE %s, TIME %s, BY %s@%s\n", FWVER_BUILD_DATE,
	       FWVER_BUILD_TIME, FWVER_WHOAMI, FWVER_HOST_NAME);

	printk("\n");
	return 0;
}

SHELL_CMD_REGISTER(fw_version, NULL, "Display PSE FW version", cmd_fw_version);

static void app_config(void)
{
	sedi_pm_switch_core_clock(CLOCK_FREQ_100M);
}

/* @brief main function
 * Enable PSE ECLite and OOB services. For both services,
 * the service main thread will be invoked in cooperative mode
 * and service main entry will invoke even before application main function.
 */
void app_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	printk("PSE ECLite and OOB services [%s]\n", CONFIG_BOARD);
}

DEFINE_USER_MODE_APP(3, USER_MODE_APP | K_PART_GLOBAL, app_main, 1024, NULL, 0, app_config);

/**
 * @}
 */

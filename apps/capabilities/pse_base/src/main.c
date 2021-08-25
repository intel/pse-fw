/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample Application for Base Image
 * This example demonstrates a simple application with shell
 * with minimum IOs and services enabled.
 * @{
 */

/**
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more details on how to
 * build the sample codes.
 */

/* Local Includes */
#include <fw_version.h>
#include <logging/log.h>
#include <pm_service.h>
#include <sys/printk.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <stdlib.h>
#include <user_app_framework/user_app_framework.h>
#include <version.h>
#include <zephyr.h>

/* Unlock value for Data Watchpoint and Trace register */
#define DWT_UNLOCK_VAL 0xC5ACCE55
/* Value of 1MHz */
#define ONE_MEG 1000000

/* @brief firmware version
 * Display PSE firmware version
 */
static int cmd_fw_version(const struct shell *shell, size_t argc, char **argv)
{
	/* Use ARG_UNUSED([arg]) to avoid compiler
	 * warnings about unused arguments
	 */
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

/* @brief ARM core clock
 * Display PSE ARM core clock speed
 */
static int cmd_arm_core_clk(const struct shell *shell, size_t argc, char **argv)
{
	volatile uint32_t cycles;
	volatile DWT_Type *dwt = DWT;
	bool locked = false;

	/* Use ARG_UNUSED([arg]) to avoid compiler
	 * warnings about unused arguments
	 */
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!dwt->CYCCNT) {
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
		dwt->LAR = DWT_UNLOCK_VAL;
		dwt->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
		locked = true;
	}

	/* Calculate ARM core speed. */
	dwt->CYCCNT = 0;
	k_busy_wait(1000000);
	cycles = dwt->CYCCNT;
	printk("Estimated ARM core speed %d MHz\n", cycles / ONE_MEG);

	if (locked) {
		CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
		dwt->LAR = 0;
		dwt->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
	}
	return 0;
}

/* @brief switch core clock
 * Switch core clock frequency
 */
static int cmd_switch_core_clock(const struct shell *shell, size_t argc,
				 char **argv)
{
	int clock_freq;

	/* Use ARG_UNUSED([arg]) to avoid compiler
	 * warnings about unused arguments
	 */
	ARG_UNUSED(shell);

	if (argc > 1) {
		/* Get command line argument. */
		clock_freq = atoi(argv[1]);
		/* Switch core clock frequency. */

		if (clock_freq == 100) {
			sedi_pm_switch_core_clock(CLOCK_FREQ_100M);
		} else if (clock_freq == 400) {
			sedi_pm_switch_core_clock(CLOCK_FREQ_400M);
		} else if (clock_freq == 500) {
			if (sedi_pm_get_hw_rev() == SEDI_HW_REV_A0) {
				printk("500 clock frequency not supported in "
				       "A0\n");
				return 0;
			}
			sedi_pm_switch_core_clock(CLOCK_FREQ_500M);

		}
	} else {
		printk("Invalid arguments: switch_core_clock <clock frequency "
		       "100/400/500>\n");
	}
	return 0;
}

SHELL_CMD_REGISTER(fw_version, NULL, "Display PSE FW version", cmd_fw_version);

SHELL_CMD_REGISTER(arm_core_clk, NULL, "Display PSE ARM Core clk speed",
		   cmd_arm_core_clk);

SHELL_CMD_REGISTER(switch_core_clock, NULL, "Switch core clock frequency",
		   cmd_switch_core_clock);

/* @brief main function
 * Enable shell and ECLite service
 */
void app_main(void *p1, void *p2, void *p3)
{
	/* Use ARG_UNUSED([arg]) to avoid compiler
	 * warnings about unused arguments
	 */
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("PSE Base Application with ECLite [%s]\n", CONFIG_BOARD);
}

DEFINE_USER_MODE_APP(3, USER_MODE_APP, app_main, 1024, NULL, 0, NULL);

/**
 * @}
 */

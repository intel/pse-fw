/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>

#include <fw_version.h>

void __weak fwver_user_print(void)
{
}

static int fwver_print(const struct device *arg)
{
	ARG_UNUSED(arg);

	printk("\nFW VERSION: %s.%s.%s\n",
			FWVER_MAJOR, FWVER_MINOR, FWVER_PATCH);
	printk("FW HW IP: %s\n", FWVER_HW_IP);
	printk("FW BUILD: DATE %s, TIME %s, BY %s@%s\n", FWVER_BUILD_DATE,
			FWVER_BUILD_TIME, FWVER_WHOAMI, FWVER_HOST_NAME);

	fwver_user_print();
	printk("\n");
	return 0;
}

SYS_INIT(fwver_print, APPLICATION, 0);

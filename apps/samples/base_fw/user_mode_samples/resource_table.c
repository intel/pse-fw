/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <kernel_structs.h>
#include <user_app_framework/user_app_config.h>
#include <user_app_framework/user_app_framework.h>

struct user_app_res_table_t
	global_resource_info[CONFIG_NUM_USER_APP_AND_SERVICE] = {
	/*PMC/System service. */
	{ .app_id = 0, .priority = SERV_BOOT_PRIO_0 },
	/* ECLite service. */
	{ .app_id = 1,
	  .priority = SERV_BOOT_PRIO_1,
	/* .sys_service = PMC_SERVICE, TODO */
	  .dev_list = { "I2C_0", "GPIO_0", "PWM_0", "TGPIO_0" },
	  .dev_cnt = 4 },
	/* OOB service. */
	{ .app_id = 2, .priority = SERV_BOOT_PRIO_2,
	/* .sys_service = PMC_SERVICE }, TODO */
	},
	/* Hello World sample App. */
	{ .app_id = 3, .priority = APP_BOOT_PRIO_1 },
	/* User Mode Sample Services */
	{ .app_id = 4, .priority = SERV_BOOT_PRIO_3 },
	{ .app_id = 5, .priority = SERV_BOOT_PRIO_4 },
	/* User Mode Sample Apps */
	{ .app_id = 6, .priority = APP_BOOT_PRIO_2 },
	{ .app_id = 7, .priority = APP_BOOT_PRIO_3 },
	{ .app_id = 8,
	  .priority = APP_BOOT_PRIO_4,
	  .dev_list = { "DMA_0", "I2S_0" },
	  .dev_cnt = 2 }
};

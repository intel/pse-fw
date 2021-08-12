/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <user_app_framework/user_app_framework.h>
#include <user_app_framework/user_app_config.h>
#include <kernel_structs.h>

struct user_app_res_table_t
	global_resource_info[CONFIG_NUM_USER_APP_AND_SERVICE] = {
	/*PMC/System service. */
	{ .app_id = 0, .priority = SERV_BOOT_PRIO_0,
	  .dev_list = { "IPC_HOST", "IPC_PMC" },
	  .dev_cnt = 2 },
	/* ECLite service. */
	{ .app_id = 1, .priority = SERV_BOOT_PRIO_1,
	  .sys_service = PMC_SERVICE,
	  .dev_list = { "I2C_0", "GPIO_0", "PWM_0" },
	  .dev_cnt = 3 },
	/* OOB service. */
	{ .app_id = 2, .priority = SERV_BOOT_PRIO_2,
	  .sys_service = PMC_SERVICE },
	/* PMC sample App. */
	{ .app_id = 3, .priority = APP_BOOT_PRIO_4,
	  .sys_service = PMC_SERVICE,
	  .dev_list = { "UART_2" },
	  .dev_cnt = 1 },
};

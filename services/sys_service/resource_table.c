/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <user_app_framework/user_app_framework.h>
#include <user_app_framework/user_app_config.h>
#include <kernel_structs.h>

__attribute__((weak)) struct user_app_res_table_t
	global_resource_info[CONFIG_NUM_USER_APP_AND_SERVICE] = {
	/*PMC/System service. */
	{ .app_id = 0, .priority = SERV_BOOT_PRIO_0,
	  .dev_list = { "IPC_HOST", "IPC_PMC"},
	  .dev_cnt = 2 },
	/* Hello World sample App. */
	{ .app_id = 1, .priority = APP_BOOT_PRIO_1 },
};

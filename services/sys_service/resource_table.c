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
#ifdef CONFIG_SYS_SERVICE
	/* PMC/System service. */
	{ .app_id = 0, .priority = SERV_BOOT_PRIO_0,
	  .dev_list = { "IPC_HOST", "IPC_PMC"},
	  .dev_cnt = 2 },
#endif
#ifdef CONFIG_ECLITE_SERVICE
	/* ECLite service. */
	{ .app_id = 1, .priority = SERV_BOOT_PRIO_1,
	  .sys_service = PMC_SERVICE | HOST_SERVICE,
	  .dev_list = { "I2C_2", "I2C_7", "IPC_HOST", "GPIO_0", "GPIO_1", "PWM_1" },
	  .dev_cnt = 6 },
#endif
#ifdef CONFIG_OOB_SERVICE
	/* OOB service. */
	{ .app_id = 2, .priority = SERV_BOOT_PRIO_2,
	  .sys_service = PMC_SERVICE | HOST_SERVICE },
#endif
	/* Hello World sample App. */
	{ .app_id = 3, .priority = APP_BOOT_PRIO_1 },
};

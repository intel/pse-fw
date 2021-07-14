/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define PMC_SERVICE (1 << 0)
#define HOST_SERVICE (1 << 1)

struct user_app_res_table_t {
	uint8_t app_id;
	int8_t priority;
	uint8_t sys_service;
	const int8_t dev_list[CONFIG_MAX_NUM_DEVICE_LIST]
				[CONFIG_MAX_DEVICE_NAME_LEN];
	uint32_t dev_cnt;
	struct k_thread  *app_handle;
};

extern struct user_app_res_table_t global_resource_info[];

static inline struct user_app_res_table_t
		*get_user_app_res_table(uint32_t app_id)
{
	uint32_t i;

	for (i = 0; i < CONFIG_NUM_USER_APP_AND_SERVICE; i++) {
		if (global_resource_info[i].app_id == app_id) {
			return &global_resource_info[i];
		}
	}
	return NULL;
}

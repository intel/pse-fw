/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_USER_APP_FRAMEWORK_H_
#define ZEPHYR_INCLUDE_USER_APP_FRAMEWORK_H_
/**
 * @brief User App Framework APIs
 * @defgroup usr_app_fw_interface User App Framework APIs
 * @{
 */
#include <zephyr.h>
#include <kernel_structs.h>
#if defined(CONFIG_USER_APP_FRAMEWORK)
#include <app_memory/app_memdomain.h>
#endif
#include <user_app_framework/user_app_utils.h>
#if (CONFIG_NUM_USER_APP_AND_SERVICE > 9)
#error Max MPU Partition reached for the current HW supports !!!
#endif

#define MAX_APP0_DEV_ACCESS_REQ (4)
#define MAX_DEV_NAME_LEN (20)

#define USER_MODE_APP (1 << 0)
#define USER_MODE_SERVICE (1 << 1)

#define K_PART_GLOBAL (1 << 2)
#define K_PART_SHARED (1 << 3)

#define APP_BOOT_PRIO_0 (0)
#define APP_BOOT_PRIO_1 (1)
#define APP_BOOT_PRIO_2 (2)
#define APP_BOOT_PRIO_3 (3)
#define APP_BOOT_PRIO_4 (4)
#define APP_BOOT_PRIO_5 (5)
#define APP_BOOT_PRIO_6 (6)
#define APP_BOOT_PRIO_7 (7)
#define APP_BOOT_PRIO_8 (8)
#define APP_BOOT_PRIO_9 (9)

#define SERV_BOOT_PRIO_0 K_PRIO_COOP(1)
#define SERV_BOOT_PRIO_1 K_PRIO_COOP(2)
#define SERV_BOOT_PRIO_2 K_PRIO_COOP(3)
#define SERV_BOOT_PRIO_3 K_PRIO_COOP(4)
#define SERV_BOOT_PRIO_4 K_PRIO_COOP(5)
#define SERV_BOOT_PRIO_5 K_PRIO_COOP(6)
#define SERV_BOOT_PRIO_6 K_PRIO_COOP(7)
#define SERV_BOOT_PRIO_7 K_PRIO_COOP(8)
#define SERV_BOOT_PRIO_8 K_PRIO_COOP(9)
#define SERV_BOOT_PRIO_9 K_PRIO_COOP(10)

#define __kernel

/* Below structure is only for Kernel internal use and App/service
 * should not use them directly.
 */
typedef void (*drv_config_func_t)(void);
struct app_info_t {
	uint32_t mode;
	void **obj_list;
	uint32_t obj_cnt;
	uint32_t stack_size;
	drv_config_func_t config_func;
	k_thread_entry_t user_app_main;
	k_thread_stack_t *app_stack;
	struct k_thread  *app_handle;
};


/**
 * @brief Statically define a user mode App.
 *
 * In case of mode is USER_MODE_APP then the App Main thread will be scheduled
 * to execution only  after all the service start execute.  But if mode is
 * USER_MODE_SERVICE then app Main thread will scheduled to execution immidiately
 * after post kernel init and before main function call.
 * @param app ID. Note: User mode App and Service have common App ID and first
 * few App IDs (0 to 2) reserved for system services.
 * @param  Mode of App whether is a user mode service or user mode App.
 * @param App main entry function.
 * @param App main Stack size in bytes.
 * @param List of kernel object which App need access with in its local threads
 * or from App main.
 * @param Total number of kernel object which App need access with in its local
 * threads or from App main.
 * @Start up init function for configure drivers, setup driver callbacks and
 * other supervisory mode only activities.
 *
 */
#define DEFINE_USER_MODE_APP(app_id, app_mode, app_main_task,		\
	stack_sizes,  kobj_list, total_obj, config_cb)		\
	k_tid_t app##app_id##_tid;					\
	static __kernel struct k_thread app##app_id##_handle;		\
	static K_THREAD_STACK_DEFINE(app##app_id##_stack, stack_sizes);	\
	struct app_info_t app##app_id##_info = {			\
		.mode = app_mode,					\
		.obj_list = kobj_list, .obj_cnt = total_obj,		\
		.app_stack = app##app_id##_stack,			\
		.config_func = config_cb,				\
		.stack_size = stack_sizes,				\
		.user_app_main = app_main_task,			\
		.app_handle =  &app##app_id##_handle }


#define GET_APP_MAIN_THREAD_ID(app_id) \
	&app##app_id##_tid

#if defined(CONFIG_USER_APP_FRAMEWORK)

#define DECLARE_APP_CONTEXT(app_id, na) \
	extern struct k_mem_partition uafw_part##app_id;
extern struct k_mem_partition uafw_part9;

USER_APP_UTIL_LISTIFY(CONFIG_NUM_USER_APP_AND_SERVICE, DECLARE_APP_CONTEXT, 0);

#define APP_USER_MODE_TASK  K_USER

#define APP_GLOBAL_VAR(app_id) \
	 K_APP_DMEM(uafw_part##app_id)

#define APP_GLOBAL_VAR_BSS(app_id) \
	K_APP_BMEM(uafw_part##app_id)

#define APP_SHARED_VAR \
	K_APP_DMEM(uafw_part9)

#define APP_SHARED_VAR_BSS \
	K_APP_BMEM(uafw_part9)
#else
#define APP_USER_MODE_TASK (0)
#define APP_GLOBAL_VAR(app_id)
#define APP_GLOBAL_VAR_BSS(app_id)
#define APP_SHARED_VAR
#define APP_SHARED_VAR_BSS
#endif
/**
 * @}
 */
#endif

/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <sys/libc-hooks.h>
#include <user_app_framework/user_app_framework.h>
#include <user_app_framework/user_app_config.h>
#include <device.h>

#define LOG_LEVEL CONFIG_APP_FRAMEWORK_LOG
#include <logging/log.h>
LOG_MODULE_REGISTER(user_app_framework);

static struct user_app_res_table_t *get_user_app_res_table(uint32_t app_id);


#define DEFINE_APP_CONTEXT(app_id, na)		     \
	extern struct app_info_t app##app_id##_info; \
	extern k_tid_t app##app_id##_tid;	     \
	K_APPMEM_PARTITION_DEFINE(uafw_part##app_id);

K_APPMEM_PARTITION_DEFINE(uafw_part9);

#define APP_INIT_PARTITION_SHARED() \
	uafw_part9.attr.rasr_attr |= \
	NORMAL_OUTER_INNER_WRITE_BACK_NON_SHAREABLE; \

#define APP_INIT_PARTITION(part_id, na)			   \
	do {							   \
		LOG_DBG("appmem_init_part_part%d\n", part_id); \
		uafw_part##part_id.attr.rasr_attr |=\
		NORMAL_OUTER_INNER_WRITE_BACK_NON_SHAREABLE; \
	} while (0);

#define START_USER_MODE_APP(app_id, na)				   \
	do {\
		if (app##app_id##_info.app_handle) {			   \
			LOG_DBG("k_thread_start %p\n",		   \
				app##app_id##_info.app_handle);	   \
			k_thread_start(app##app_id##_info.app_handle);	   \
		} \
	} while (0);

#define DEFINE_WEEK_APP_CTX(app_id, na)					\
	__attribute__((weak)) struct app_info_t app##app_id##_info = {	\
					 .user_app_main = NULL };	\
	__attribute__((weak)) k_tid_t app##app_id##_tid;

USER_APP_UTIL_LISTIFY(CONFIG_NUM_USER_APP_AND_SERVICE, DEFINE_WEEK_APP_CTX, 0);


/*TODO Need to optimize this macro. */
#define DEFINE_INIT_USER_MODE_APP(app_id, na)				\
	struct k_mem_domain dom##app_id;				\
	static inline void init_user_mode_app##app_id(void)		\
	{								\
		LOG_DBG("%s\n", __func__);				\
		struct user_app_res_table_t *global_res =		\
				get_user_app_res_table(app_id);	        \
		if (!app##app_id##_info.user_app_main || !global_res) {	\
			return;						\
		}							\
		if ((app##app_id##_info.mode & USER_MODE_SERVICE) &&    \
		    (global_res->priority < SERV_BOOT_PRIO_0 ||	\
			global_res->priority > SERV_BOOT_PRIO_9)) {	\
			LOG_ERR(					\
			"Invalid service priority given for app_id:%d\n",   \
					 app_id);			    \
			return;						    \
		}							    \
		if ((app##app_id##_info.mode & USER_MODE_APP) &&	    \
		    (global_res->priority < APP_BOOT_PRIO_0 ||		    \
			 global_res->priority > APP_BOOT_PRIO_9)) {	    \
			LOG_ERR(					    \
			"Invalid App priority given for app_id:%d\n", app_id); \
			return;						       \
		}							       \
		app##app_id##_tid = k_thread_create(app##app_id##_info	       \
							.app_handle,	       \
						    app##app_id##_info	       \
							.app_stack,	       \
						    app##app_id##_info	       \
							.stack_size,           \
						    app##app_id##_info	       \
							.user_app_main,        \
						    NULL, NULL, NULL,	       \
				global_res->priority, K_USER, K_FOREVER);      \
		global_res->app_handle = app##app_id##_info.app_handle;	       \
		if (app##app_id##_info.obj_list) {			       \
			for (int dev_cnt = 0; dev_cnt < app##app_id##_info     \
				.obj_cnt; dev_cnt++) {			       \
				k_object_access_grant(app##app_id##_info       \
							.obj_list[dev_cnt],    \
						      app##app_id##_info       \
							.app_handle);          \
				k_thread_system_pool_assign(		       \
							app##app_id##_tid);    \
			}						       \
		}							       \
		k_mem_domain_init(&dom##app_id, 0, NULL);		       \
		k_mem_domain_add_partition(&dom##app_id, &z_libc_partition);   \
		if ((app##app_id##_info.mode & K_PART_GLOBAL)) {               \
			k_mem_domain_add_partition(&dom##app_id,               \
							 &uafw_part##app_id);  \
		}                                                              \
		if ((app##app_id##_info.mode & K_PART_SHARED)) {	       \
			k_mem_domain_add_partition(&dom##app_id, &uafw_part9); \
		}							       \
		k_mem_domain_add_thread(&dom##app_id, app##app_id##_tid);      \
	}


#define INIT_ALL_USER_MODE_APP(app_id, na)		 \
	do {						 \
		LOG_DBG("INIT_ALL_USER_MODE_APP\n"); \
		init_user_mode_app##app_id();		 \
	} while (0);

#define INVOKE_APP_CONFIG_CALLBACK(app_id, na)		  \
	do {						  \
		if (app##app_id##_info.config_func) {	  \
			app##app_id##_info.config_func(); \
		}					  \
	} while (0);


USER_APP_UTIL_LISTIFY(CONFIG_NUM_USER_APP_AND_SERVICE, DEFINE_APP_CONTEXT, 0)
USER_APP_UTIL_LISTIFY(CONFIG_NUM_USER_APP_AND_SERVICE,
					DEFINE_INIT_USER_MODE_APP, 0)

/* Grant driver access to each App from global resource table. */
static void grant_driver_access(void)
{
	LOG_DBG("%s\n", __func__);
	const struct device *dev_handle;
	uint32_t app_id;

	for (int i = 0; i <  CONFIG_NUM_USER_APP_AND_SERVICE; i++) {
		app_id = global_resource_info[i].app_id;
		if (app_id >= CONFIG_NUM_USER_APP_AND_SERVICE) {
			LOG_ERR(
			"invalid App ID given in global_resource_info[%d]\n",
				 i);
			continue;
		}
		if (!global_resource_info[app_id].dev_list ||
			!global_resource_info[app_id].dev_cnt) {
			LOG_DBG(
			"no device access request for App:%d\n",
				(uint32_t)app_id);
			continue;
		}
		LOG_DBG("app_id: %d, No of device:%d\n",
			(uint32_t)app_id, (uint32_t)global_resource_info[app_id].dev_cnt);
		for (int dev_cnt = 0;
			dev_cnt < global_resource_info[app_id].dev_cnt;
			dev_cnt++) {
			dev_handle = device_get_binding(
					global_resource_info[app_id]
					.dev_list[dev_cnt]);
			LOG_DBG("Device:%s, handle:%p\n",
					log_strdup(global_resource_info[app_id]
					.dev_list[dev_cnt]),
					dev_handle);
			if (dev_handle) {
				k_object_access_grant(dev_handle,
					global_resource_info[app_id]
					.app_handle);
			} else {
				LOG_ERR("error device bind req for: %s\n",
					log_strdup(global_resource_info[app_id]
						.dev_list[dev_cnt]));
			}
		}
	}
}
static int user_app_init(const struct device *unused)
{
	LOG_DBG("%s:%p\n", __func__, unused);

	USER_APP_UTIL_LISTIFY(CONFIG_NUM_USER_APP_AND_SERVICE,
					APP_INIT_PARTITION, 0);
	APP_INIT_PARTITION_SHARED();

	USER_APP_UTIL_LISTIFY(CONFIG_NUM_USER_APP_AND_SERVICE,
					INIT_ALL_USER_MODE_APP, 0);

	grant_driver_access();

	USER_APP_UTIL_LISTIFY(CONFIG_NUM_USER_APP_AND_SERVICE,
					INVOKE_APP_CONFIG_CALLBACK, 0);
	USER_APP_UTIL_LISTIFY(CONFIG_NUM_USER_APP_AND_SERVICE,
					START_USER_MODE_APP, 0);

	return 0;
}


SYS_INIT(user_app_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

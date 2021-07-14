/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NUM_USER_APP_AND_SERVICE)
#include <init.h>
#include <user_app_framework/user_app_framework.h>
#include <user_app_framework/user_app_config.h>

#define LOG_LEVEL CONFIG_APP_FRAMEWORK_LOG
#include <logging/log.h>
LOG_MODULE_REGISTER(non_user_app_framework);

#define START_USER_MODE_APP(app_id, na)				       \
	do {							       \
		if (app##app_id##_info.app_handle) {		       \
			LOG_DBG("k_thread_start %p\n",	       \
				    app##app_id##_info.app_handle);    \
			k_thread_start(app##app_id##_info.app_handle); \
		}						       \
	} while (0);
#define DEFINE_WEEK_APP_CTX(app_id, na)				       \
	__attribute__((weak)) struct app_info_t app##app_id##_info = { \
		.user_app_main = NULL };			       \
	__attribute__((weak)) k_tid_t app##app_id##_tid;

USER_APP_UTIL_LISTIFY(CONFIG_NUM_USER_APP_AND_SERVICE, DEFINE_WEEK_APP_CTX, 0);


#define DEFINE_INIT_USER_MODE_APP(app_id, na)				  \
	static inline void init_user_mode_app##app_id(void)		  \
	{								  \
		LOG_DBG("%s\n", __func__);				  \
		struct user_app_res_table_t *global_res =		  \
			get_user_app_res_table(app_id);			  \
		if (!app##app_id##_info.user_app_main || !global_res) {	  \
			return;						  \
		}							  \
		if ((app##app_id##_info.mode & USER_MODE_SERVICE) &&	  \
		    (global_res->priority < SERV_BOOT_PRIO_0 ||		  \
		     global_res->priority > SERV_BOOT_PRIO_9)) {	  \
			LOG_ERR(	\
		"Invalid service priority given for app_id:%d\n",	  \
					app_id);			  \
			return;						  \
		}							  \
		if ((app##app_id##_info.mode & USER_MODE_APP) &&	  \
		    (global_res->priority < APP_BOOT_PRIO_0 ||		  \
		     global_res->priority > APP_BOOT_PRIO_9)) {		  \
			LOG_ERR(					  \
				"Invalid App priority given for app_id:%d\n",\
				app_id);				  \
			return;						  \
		}							  \
		app##app_id##_tid = k_thread_create(app##app_id##_info	  \
						    .app_handle,	  \
						    app##app_id##_info	  \
						    .app_stack,		  \
						    app##app_id##_info	  \
						    .stack_size,	  \
						    app##app_id##_info	  \
						    .user_app_main, NULL, \
						    NULL, NULL,		  \
						    global_res->priority, \
						    0, K_FOREVER);	  \
		global_res->app_handle = app##app_id##_info.app_handle;	  \
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

USER_APP_UTIL_LISTIFY(CONFIG_NUM_USER_APP_AND_SERVICE,
				DEFINE_INIT_USER_MODE_APP, 0)

static int user_app_init(const struct device *unused)
{
	LOG_DBG("%s\n", __func__);
	USER_APP_UTIL_LISTIFY(CONFIG_NUM_USER_APP_AND_SERVICE,
					INIT_ALL_USER_MODE_APP, 0);
	USER_APP_UTIL_LISTIFY(CONFIG_NUM_USER_APP_AND_SERVICE,
					INVOKE_APP_CONFIG_CALLBACK, 0);
	USER_APP_UTIL_LISTIFY(CONFIG_NUM_USER_APP_AND_SERVICE,
					START_USER_MODE_APP, 0);
	return 0;
}

SYS_INIT(user_app_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif

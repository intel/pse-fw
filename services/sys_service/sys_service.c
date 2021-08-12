/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel_structs.h>
#include <string.h>
#include <user_app_framework/user_app_framework.h>
#include <user_app_framework/user_app_config.h>
#include "pse_sys_service.h"

#include "pmc_service/pmc_service_common.h"
#include "host_service/host_service_common.h"
#if defined(CONFIG_WOL_SERVICE)
#include "wol_service.h"
#endif
#include "pm_service.h"

static void sys_config(void)
{
#if defined(CONFIG_PMC_SERVICE)
	pmc_config();
#endif
#if defined(CONFIG_HOST_SERVICE)
	host_config();
#endif
#if defined(CONFIG_WOL_SERVICE)
	wol_config();
#endif
}

static void sys_service_main(void *p1, void *p2, void *p3)
{
#if defined(CONFIG_PMC_SERVICE)
	pmc_service_init();
#endif
#if defined(CONFIG_HOST_SERVICE)
	host_service_init();
#endif
#if defined(CONFIG_WOL_SERVICE)
	wol_service_init();
#endif
}

/* Add the requried sys service local kernel object pointer into list. */
static const void *obj_list[] = {
#if defined(CONFIG_PMC_SERVICE)
	PMC_K_OBJ_LIST,
#endif
#if defined(CONFIG_HOST_SERVICE)
	HOST_K_OBJ_LIST
#endif
};
/* Define system service. */
DEFINE_USER_MODE_APP(0, USER_MODE_SERVICE | K_PART_GLOBAL, sys_service_main,
		     1024, (void **)obj_list, SYS_K_OBJ_LIST_SIZE, sys_config);

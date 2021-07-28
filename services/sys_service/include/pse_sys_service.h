/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYS_SERVICE_H
#define SYS_SERVICE_H

/*
 * if adding a service, grow the size here.
 * if service not configurated set according size to 0
 */
#define SYS_K_OBJ_LIST_SIZE (HOST_K_OBJ_LIST_SIZE)
#define SYS_SERV_MAIN_STACK_SIZE (1024)

#if defined(CONFIG_PMC_SERVICE)
#include "pmc_service.h"
#endif
#include "host_ipc_service.h"

#endif


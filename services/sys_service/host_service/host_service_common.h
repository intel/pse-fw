/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HOST_SERVICE_COMMON_H
#define HOST_SERVICE_COMMON_H

#if defined(CONFIG_HOST_SERVICE)
extern __kernel struct k_thread host_service_thread;
K_THREAD_STACK_EXTERN(host_service_stack);
extern __kernel struct k_sem sem_ipc_read;

#define HOST_K_OBJ_LIST \
    &sem_ipc_read, &host_service_thread, &host_service_stack

#define HOST_K_OBJ_LIST_SIZE (3)

void host_config(void);
void host_service_init(void);

#ifdef CONFIG_HECI
int heci_init(struct device *arg);
#endif
#ifdef CONFIG_SYS_MNG
int mng_and_boot_init(struct device *dev);
#endif


#else
#define HOST_K_OBJ_LIST []
#define HOST_K_OBJ_LIST_SIZE (0)
#endif

#endif

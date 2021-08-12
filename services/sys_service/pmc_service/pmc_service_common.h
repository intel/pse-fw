/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PMC_SERVICE_COMMON_H
#define PMC_SERVICE_COMMON_H

#if defined(CONFIG_PMC_SERVICE)
extern __kernel struct k_thread pmc_msg_thread_handle;
K_THREAD_STACK_EXTERN(pmc_msg_thread_stack);
extern __kernel struct k_fifo pmc_serv_fifo;
extern __kernel struct k_mutex pmc_msg_mutex;
extern __kernel struct k_sem pmc_msg_alert;

#define PMC_K_OBJ_LIST &pmc_msg_mutex, &pmc_msg_alert, \
	&pmc_serv_fifo, &pmc_msg_thread_handle, &pmc_msg_thread_stack

#define PMC_K_OBJ_LIST_SIZE (5)

void pmc_config(void);
void pmc_service_init(void);

#else

#define PMC_K_OBJ_LIST []
#define PMC_K_OBJ_LIST_SIZE (0)

#endif

#endif

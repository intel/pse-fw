/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IPC_SERVICE_H
#define __IPC_SERVICE_H
/**
 * @brief IPC Services APIs
 * @defgroup ipc_interface IPC Service APIs
 * @{
 */
#include <kernel.h>
#include <errno.h>
#include <drivers/ipc.h>

/**
 * @brief
 * callback function being called to handle protocol message
 * @retval 0 If successful.
 */
typedef int (*ipc_msg_handler_f)(const struct device *ipc_dev, uint32_t drbl);

/**
 * @brief
 * add function cb in pse-host ipc service task
 * to handle ipc message.
 * @retval 0 If successful.
 */
int host_protocol_register(uint8_t protocol_id,  ipc_msg_handler_f handler);

/*
 * @brief
 * request host access to send ipc or DMA to DDR memory
 * host_access_dereq() must be paired after every successful host_access_req(),
 * otherwise, will lead to RTD3 instabilty
 * @param timeout: max timeout wait for access request(in milliseconds)
 * @retval handler for the request, <0 if failure
 */
int host_access_req(int32_t timeout);

/*
 * @brief
 * release host access
 * @param request_handler: handler id returned by host_access_req
 * @retval <0 if failure
 */
int host_access_dereq(int request_handler);

/**
 * @}
 */

#endif /* __IPC_SERVICE_H */

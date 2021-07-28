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
/**
 * @}
 */

#endif /* __IPC_SERVICE_H */

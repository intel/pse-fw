/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief This file exposes APIs to for GPIO and interrupt service
 */

#ifndef _ECLITE_SERVICE_H_
#define _ECLITE_SERVICE_H_
#include <logging/log.h>
#include <stdint.h>

/**
 * @brief Function initializes GPIO and does interrupt callback registration.
 *
 * @param platform_device_ptr is eclite device list
 * @param platform_gpio_ptr is GPIO table
 * @param no_of_devices is number of devices EClite application manages.
 * @param no_of_gpios is number of GPIOs connected to device.
 *
 * @retval : error.
 */
int eclite_service_gpio_config(void *platform_device_ptr[],
			       void *platform_gpio_ptr[],
			       uint32_t no_of_devices,
			       uint32_t no_of_gpios);

/**
 * @brief This function does application initialization.
 *
 * @param port is GPIO device structure return from device_get_binding().
 * @param cb hold GPIO callback data for interrupt.
 * @param pins hold GPIO bit mask of the interrupt.
 *
 * @retval : None.
 */
void eclite_service_isr(const struct device *port,
			struct gpio_callback *cb, uint32_t pins);

#endif /*_ECLITE_SERVICE_H_ */

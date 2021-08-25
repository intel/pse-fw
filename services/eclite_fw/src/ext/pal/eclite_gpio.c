/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <drivers/gpio.h>
#include "eclite_hw_interface.h"
#include <device.h>
#include "platform.h"
#include "common.h"

LOG_MODULE_REGISTER(eclite_gpio, CONFIG_ECLITE_LOG_LEVEL);

struct device *eclite_get_gpio_device(const char *name)
{
	return (struct device *)device_get_binding(name);
}

int eclite_gpio_configure(void *dev, uint32_t pin, uint32_t flags)
{
	const struct device *gpio_dev = (const struct device *)dev;

	return gpio_pin_configure(gpio_dev, (gpio_pin_t)pin, flags);
}

int eclite_gpio_register_callback(void *dev, void *cb,
				  gpio_callback_handler_t handler,
				  uint32_t pin)
{
	const struct device *gpio_dev = (const struct device *)dev;
	struct gpio_callback *callback = cb;
	int ret;

	gpio_init_callback(callback, handler, BIT(pin));
	ret = gpio_add_callback(gpio_dev, callback);
	if (ret) {
		LOG_ERR("Callback setup failed");
		return ret;
	}
	return 0;
}

int eclite_set_gpio(void *dev, uint32_t pin, uint32_t value)
{
	const struct device *gpio_dev = (const struct device *)dev;

	return gpio_pin_set_raw(gpio_dev, pin, value);
}

int eclite_get_gpio(void *dev, uint32_t pin, uint32_t *value)
{
	const struct device *gpio_dev = (const struct device *)dev;

	*value = gpio_pin_get_raw(gpio_dev, pin);
	return SUCCESS;
}

int eclite_gpio_pin_enable_callback(void *dev, uint32_t pin, uint32_t flags)
{
	const struct device *gpio_dev = (const struct device *)dev;

	return gpio_pin_interrupt_configure(gpio_dev, (gpio_pin_t)pin, flags);
}

int eclite_gpio_pin_disable_callback(void *dev, uint32_t pin)
{
	const struct device *gpio_dev = (const struct device *)dev;
	uint32_t flags = GPIO_INPUT | GPIO_INT_DISABLE;

	return gpio_pin_interrupt_configure(gpio_dev, pin, flags);
}

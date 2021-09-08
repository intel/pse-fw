/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <device.h>
#include <drivers/gpio.h>
#include <stdint.h>
#include <zephyr.h>
#include <sys/__assert.h>

#include "eclite_device.h"
#include "platform.h"
#include "common.h"
#include "eclite_dispatcher.h"
#include "eclite_hw_interface.h"

LOG_MODULE_REGISTER(eclite_service, CONFIG_ECLITE_LOG_LEVEL);

void eclite_service_isr(const struct device *port,
			struct gpio_callback *cb, uint32_t pins)
{

	/* Run through the gpio mapping in eclite device list
	 * Check the device Type associated with GPIO Event
	 * Queue the event accordingly.
	 */
	struct dispatcher_queue_data event;
	int ret;

	event.event_type = GPIO_EVENT;
	event.data = find_bit_position(pins);

	eclite_gpio_pin_disable_callback((void *)port, find_bit_position(pins));
	ret = eclite_post_dispatcher_event(&event);
	if (ret) {
		LOG_ERR("Error Posting an event");
	}
}

int eclite_service_gpio_config(struct eclite_device *eclite_dev_list[],
			       struct platform_gpio_list plt_gpio_list[],
			       uint32_t no_of_devices, uint32_t no_plt_gpio)
{
	uint32_t gpio_pin_num;
	int gpio_pin_flag;
	int ret;

	/* Application must fail if it cannot configure gpios. It's better to
	 * than silently ignore error.
	 */
	__ASSERT(no_of_devices > 0, "no_of_devices cannot be 0");

	/* Run through the global list. */
	for (int j = 0; j < no_plt_gpio; j++) {
		for (int i = 0; i < no_of_devices; i++) {
			void *gpio_dev =
				eclite_dev_list[i]->hw_interface->gpio_dev;
			const struct platform_gpio_config *gpio_cfg =
				(const struct platform_gpio_config *)
				eclite_dev_list[i]->hw_interface->gpio_config;

			if (!gpio_dev) {
				ECLITE_LOG_DEBUG("No GPIO for %s()",
						 eclite_dev_list[i]->name);
				continue;
			}
			gpio_pin_num = gpio_cfg->gpio_no;
			gpio_pin_flag = gpio_cfg->gpio_config.dir |
					gpio_cfg->gpio_config.pull_down_en |
					gpio_cfg->gpio_config.intr_type;

			if (gpio_pin_num == CHARGER_GPIO) {
				uint32_t pin_value;

				pin_value = gpio_pin_get_raw(
					(struct device *)gpio_dev,
				        gpio_pin_num);
				if (pin_value) {
					gpio_pin_flag &=
						~(GPIO_ACTIVE_HIGH);
				}
				else {
					gpio_pin_flag |= (GPIO_ACTIVE_HIGH);
				}
			}

			if (gpio_pin_num == plt_gpio_list[j].gpio_no) {
				/* Configure each device gpio pin */
				ret = eclite_gpio_configure(gpio_dev,
							    gpio_pin_num,
							    gpio_pin_flag);
				if (ret) {
					LOG_ERR("GPIO: %u configure err",
						gpio_pin_num);
					return ret;
				}
				ret = eclite_gpio_register_callback(
					gpio_dev,
					(void *)&
					gpio_cfg->eclite_service_isr_data,
					eclite_service_isr,
					gpio_pin_num);
				if (ret) {
					LOG_ERR("Register callback failed");
					return ret;
				}

				ret = eclite_gpio_pin_enable_callback(gpio_dev,
						gpio_pin_num, gpio_pin_flag);
				if (ret) {
					LOG_ERR("Enabling callback failed");
					return ret;
				}
			}
		}
	}
	return 0;
}

/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file
 * @brief Sample Application for GPIO driver.
 * This sample shows how to configure gpio pins to output direction to control
 * pin levels, and also how to configure pins to input direction to trigger
 * interrupts.
 * The sample uses 2 pins, one as output pin, and the other
 * as input pin. Connect these 2 pins, output pin will toggle the level, this
 * would trigger interrupts for the input pin.
 *
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more
 * details how to build the sample codes.
 *
 * @brief Hardware setup.
 * Please refer section 3.8.1 in “IntelPSE_SDK_User_Guide” for
 * more details for GPIO hardware setup.
 * @{
 */

/* Local Includes */
#include <drivers/gpio.h>
#include <sys/printk.h>
#include <zephyr.h>

/* GPIO output pin device used for this sample */
#define GPIO_OUT_DRV_NAME "GPIO_0"
/* GPIO input pin device used for this sample */
#define GPIO_IN_DRV_NAME "GPIO_1"
/* GPIO output pin number used for this sample */
#define GPIO_OUT_PIN (26)
/* GPIO input pin number used for this sample */
#define GPIO_INT_PIN (17)

/* Input gpio pin callback function */
void gpio_callback(const struct device *port, struct gpio_callback *cb,
		   uint32_t pins)
{
	printk("Pin %d triggered\n", GPIO_INT_PIN);
}

/* GPIO callback while trigger interrupt */
static struct gpio_callback gpio_cb;

int main(int argc, char *argv[])
{
	const struct device *gpio_out_dev, *gpio_in_dev;
	int ret = 0;
	int toggle = 1;

	/* Get the device handle for GPIO device by 'GPIO_OUT_DRV_NAME' */
	gpio_out_dev = device_get_binding(GPIO_OUT_DRV_NAME);
	if (!gpio_out_dev) {
		printk("Cannot find %s!\n", GPIO_OUT_DRV_NAME);
		return ret;
	}

	/* Get the device handle for GPIO device by 'GPIO_IN_DRV_NAME' */
	gpio_in_dev = device_get_binding(GPIO_IN_DRV_NAME);
	if (!gpio_in_dev) {
		printk("Cannot find %s!\n", GPIO_IN_DRV_NAME);
		return ret;
	}

	/* GPIO Pin configuration
	 * GPIO PIN DIRECTION    : Output
	 */
	ret = gpio_pin_configure(gpio_out_dev, GPIO_OUT_PIN, GPIO_OUTPUT);
	if (ret) {
		printk("Error configuring pin %d!\n", GPIO_OUT_PIN);
		return ret;
	}

	/* GPIO Pin configuration
	 * GPIO PIN DIRECTION    : Input
	 * PIN INTERRUPT: Enable
	 * TRIGGER METHOD: Edge trigger
	 * TRIGGER CONDITION: Raising edge
	 */
	ret = gpio_pin_configure(gpio_in_dev, GPIO_INT_PIN, GPIO_INPUT);

	if (ret) {
		printk("Error configuring pin %d!\n", GPIO_INT_PIN);
		return ret;
	}

	ret = gpio_pin_interrupt_configure(gpio_in_dev, GPIO_INT_PIN,
					GPIO_INT_EDGE_RISING);
	/* Init user callback function */
	gpio_init_callback(&gpio_cb, gpio_callback, BIT(GPIO_INT_PIN));

	/* Add user callback with the GPIO device */
	ret = gpio_add_callback(gpio_in_dev, &gpio_cb);
	if (ret) {
		printk("Cannot setup callback!\n");
		return ret;
	}

	while (1) {
		printk("Toggling pin %d\n", GPIO_OUT_PIN);
		/* Toggle the output pin */
		ret = gpio_pin_set(gpio_out_dev, GPIO_OUT_PIN, toggle);
		if (ret) {
			printk("Error set pin %d!\n", GPIO_OUT_PIN);
			return ret;
		}

		if (toggle) {
			toggle = 0;
		} else {
			toggle = 1;
		}

		k_sleep(K_SECONDS(2));
	}
}

/**
 * @}
 */

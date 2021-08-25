/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bq24610.h"
#include "platform.h"
#include "charger_framework.h"
#include "eclite_device.h"
#include "eclite_hw_interface.h"
#include <drivers/gpio.h>
#include "common.h"

LOG_MODULE_REGISTER(charger, CONFIG_ECLITE_LOG_LEVEL);

/* device specific interface */
struct bq24610_interface {
	/* Device pointer for the CHARGER_CONTROL gpio.*/
	void *gpio_cntlr;
};

int bq24610_en_charging(void *device)
{
	struct eclite_device *dev = device;
	uint32_t charging_status = 0;
	struct bq24610_interface *gpio_dev = dev->hw_interface->device;

	eclite_get_gpio(gpio_dev->gpio_cntlr, CHARGER_GPIO_CE,
			&charging_status);

	/* don't re-enable charging if charging is enabled */
	if (charging_status) {
		return SUCCESS;
	}

	return eclite_set_gpio(dev->hw_interface->gpio_dev, CHARGER_GPIO_CE,
			       ENABLE_CHARGING);
}

static int bq24610_dis_charging(void *device)
{
	struct eclite_device *dev = device;
	struct bq24610_interface *gpio_dev = dev->hw_interface->device;

	return eclite_set_gpio(gpio_dev->gpio_cntlr, CHARGER_GPIO_CE,
			       DISABLE_CHARGING);
}

static int bq24610_check_ac_present(void *device)
{
	struct eclite_device *dev = device;
	struct charger_driver_data *charger_data = dev->driver_data;

	return eclite_get_gpio(dev->hw_interface->gpio_dev,
			       CHARGER_GPIO_AC_PRESENT,
			       &charger_data->ac_present);
}

static int bq24610_get_charger_status(void *device, uint16_t *status)
{
	*status = 0;
	/* Reference API, As we don't have I2C based read/write,
	 * not implemented
	 */
	return 0;
}

static enum device_err_code bq24610_isr(void *device)
{
	struct eclite_device *bq24610_eclite_dev = device;

	/* isr handling*/
	bq24610_check_ac_present(bq24610_eclite_dev);

	return SUCCESS;
}

static enum device_err_code bq24610_init(void *bq24610_device)
{
	struct eclite_device *dev = bq24610_device;
	struct bq24610_interface *gpio_dev = dev->hw_interface->device;

	ECLITE_LOG_DEBUG("%s(): Charger driver init\n", __func__);
	dev->hw_interface->gpio_dev = (struct device *)
				      eclite_get_gpio_device(CHARGER_GPIO_NAME);

	gpio_dev->gpio_cntlr = (struct device *)
			       eclite_get_gpio_device(CHARGER_CONTROL_NAME);
	eclite_gpio_configure(gpio_dev->gpio_cntlr, CHARGER_CONTROL,
			      GPIO_OUTPUT);

	ECLITE_LOG_DEBUG("%s(): GPIO device driver address - %d\n", __func__,
			 (int)dev->hw_interface->gpio_dev);

	return SUCCESS;
}

/* Battrey Fuelguage interface APIs. */
static APP_GLOBAL_VAR(1) struct charger_driver_api bq24610_api = {
	.enable_charging = bq24610_en_charging,
	.disable_charging = bq24610_dis_charging,
	.check_ac_present = bq24610_check_ac_present,
	.get_charger_status = bq24610_get_charger_status,
};

static APP_GLOBAL_VAR(1) struct bq24610_interface bq24610_dev;
/* Battery Fuelguage interface APIs. */
static APP_GLOBAL_VAR_BSS(1) struct charger_driver_data bq24610_data;

/* HW configuration for charger */
static APP_GLOBAL_VAR(1) struct hw_configuration bq24610_hw = {
	.device = &bq24610_dev,
};

/* Device structure for bq40z40. */
APP_GLOBAL_VAR(1) struct eclite_device bq24610_device_charger = {
	.name = "BQ24610_device_CHG",
	.device_typ = DEV_CHG,
	.init = bq24610_init,
	.driver_api = &bq24610_api,
	.driver_data = &bq24610_data,
	.isr = bq24610_isr,
	.hw_interface = &bq24610_hw,
};

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <eclite_device.h>
#include <thermal_framework.h>
#include <eclite_hw_interface.h>
#include <platform.h>
#include "fan.h"

LOG_MODULE_REGISTER(fan, CONFIG_ECLITE_LOG_LEVEL);

#define TEST_PARAM_PWM 50

struct fan_interface {
	void *pwm_dev;
	void *tacho_dev;
	uint8_t pwm_pin;
	uint32_t pwm_frequency;
};

static int fan_update_pwm(void *fan_dev, int16_t *data)
{
	int ret;
	struct eclite_device *fan_device = fan_dev;
	struct thermal_driver_data *fan_data = fan_device->driver_data;
	struct fan_interface *fan_hw = fan_device->hw_interface->device;
	uint32_t pulse;

	ARG_UNUSED(fan_data);

	pulse = fan_hw->pwm_frequency * (*data) / 100;

	if (!fan_hw->pwm_dev) {
		LOG_ERR("PWM device not found");
		return ERROR;
	}

	ret = eclite_write_pwm(fan_hw->pwm_dev, fan_hw->pwm_pin,
			       fan_hw->pwm_frequency, pulse);
	if (ret) {
		LOG_ERR("Fan speed update error: %d", ret);
		return ret;
	}

	return 0;
}

static int fan_read_pwm(void *tacho_dev, int16_t *data)
{
	int ret;

	if (tacho_dev == NULL) {
		LOG_ERR("No tacho device");
		return ERROR;
	}
	struct eclite_device *tacho_device = tacho_dev;
	struct thermal_driver_data *tacho_data = tacho_device->driver_data;
	struct fan_interface *tacho_hw = tacho_device->hw_interface->device;

	if (!tacho_hw->tacho_dev) {
		LOG_WRN("TACHO device not found");
		return ERROR;
	}

	if (tacho_data->rotation) {
		ret = eclite_read_tacho(tacho_hw->tacho_dev, (uint32_t *)data);
		if (ret) {
			LOG_ERR("Tacho reading failed.%d", ret);
			return ret;
		}
	} else {
		ECLITE_LOG_DEBUG("Tacho not updated");
	}
	ECLITE_LOG_DEBUG("TACHO: %d", *data);
	return 0;
}

static enum device_err_code fan_isr(void *fan_dev)
{
	/* Referene API, no isr for fan in this platform  */
	return 0;
}

static enum device_err_code fan_init(void *fan_dev)
{
	struct eclite_device *fan_device = fan_dev;

#ifdef CONFIG_TEST_MODE
	struct thermal_driver_data *fan_data = fan_device->driver_data;
#endif
	struct fan_interface *fan_interface = fan_device->hw_interface->device;
	int ret = DEV_INIT_FAILED;

	ECLITE_LOG_DEBUG("Fan Initialization starts");

	fan_interface->pwm_dev = (struct device *)
				 eclite_get_pwm_device(ECLITE_PWM_DEV);
	fan_interface->tacho_dev = (struct device *)
				   eclite_get_tacho_device(ECLITE_TACHO_DEV);

	if (fan_interface->pwm_dev != NULL) {
		eclite_init_pwm(fan_interface->pwm_dev, NULL);
		fan_interface->pwm_pin = PWM_PIN;
	}

	if (fan_interface->tacho_dev != NULL) {
		eclite_init_tacho(fan_interface->tacho_dev, NULL);
#ifdef CONFIG_TEST_MODE
		fan_data->rotation = TEST_PARAM_PWM;
		fan_update_pwm(fan_dev, &fan_data->rotation);
		fan_read_pwm(fan_dev, &fan_data->tacho);
#endif
	}

	if (fan_interface->pwm_dev != NULL ||
	    fan_interface->tacho_dev != NULL) {
		ret = DEV_OPS_SUCCESS;
	}

	return ret;
}

/* FAN Control APIs. */
static APP_GLOBAL_VAR(1) struct thermal_driver_api fan_api = {
	.read_data = fan_read_pwm,
	.write_data = fan_update_pwm,
};

/* FAN.*/
static APP_GLOBAL_VAR_BSS(1) struct thermal_driver_data fan_data;
static APP_GLOBAL_VAR(1) struct fan_interface fan_dev_interface = {
#ifdef CONFIG_TEST_MODE
	.pwm_frequency = ECLITE_PWM_FREQ,
#else
	.pwm_frequency = ECLITE_PWM_FREQ,
#endif
};
static APP_GLOBAL_VAR(1) struct hw_configuration fan0_interface = {
	.device = &fan_dev_interface,
};

APP_GLOBAL_VAR(1) struct eclite_device fan_device = {
	.name = "FAN",
	.device_typ = DEV_FAN,
	.instance = INSTANCE_1,
	.init = fan_init,
	.driver_api = &fan_api,
	.driver_data = &fan_data,
	.isr = fan_isr,
	.hw_interface = &fan0_interface,
};

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "eclite_device.h"
#include "eclite_hw_interface.h"
#include "thermal_framework.h"
#include "tmp102.h"
#include "platform.h"

LOG_MODULE_REGISTER(tmp102, CONFIG_ECLITE_LOG_LEVEL);

#define TEMP_ALERT_TOLERANCE 3

/* private bus/device/non-isr interface.*/
struct tmp102_interface {
	void *i2c_dev;
	uint16_t crit_temp;
};

static int tmp102_config(void *tmp102_dev, int16_t *data)
{
	int ret;
	struct eclite_device *thermal_device = tmp102_dev;
	struct thermal_driver_data *thermal_data = thermal_device->driver_data;
	struct tmp102_interface *dev_interface;
	uint16_t temp = 0;

	dev_interface = thermal_device->hw_interface->device;
	ret = eclite_i2c_read_word(dev_interface->i2c_dev,
				   thermal_data->i2c_slave,
				   TMP102_CONFIGURATION_REG, &temp);
	if (ret) {
		LOG_ERR("Failed to read configuration: %d", ret);
		return ret;
	}

	temp |= *data;
	ret = eclite_i2c_write_word(dev_interface->i2c_dev,
				    thermal_data->i2c_slave,
				    TMP102_CONFIGURATION_REG, temp);
	if (ret) {
		LOG_ERR("Failed to update configuration: %d", ret);
		return ret;
	}
	return 0;
}

static int tmp102_get_status(void *tmp102_dev, int16_t *data)
{
	struct eclite_device *thermal_device = tmp102_dev;
	struct thermal_driver_data *thermal_data = thermal_device->driver_data;
	struct tmp102_interface *dev_interface =
		thermal_device->hw_interface->device;

	return eclite_i2c_read_word(dev_interface->i2c_dev,
				    thermal_data->i2c_slave,
				    TMP102_CONFIGURATION_REG, data);
}

static void tmp102_convert_temperature(int16_t *data, int8_t format,
				       int8_t mode)
{
	int16_t temperature = 0;

	/* ref: TMP102 datasheet , section 7.5.4 for data format
	 * Equation (8 - mode) computes shift. During this shift unused bit and
	 * fraction data are removed or being padded as 0s.
	 *
	 * Below are shift values needed for different modes.
	 * Mode(val)		8 - mode	Shift
	 * Normal mode(0)	8 - 0 =		8
	 * Extended mode(1)	8 - 1 =		7
	 */
	if ((mode == TMP102_EM_NORAML_MODE) |
	    (mode == TMP102_EM_EXTENDED_MODE)) {
		if (format == TMP102_READABLE_FORMAT) {
			/* remove fractional part and unwanted bits.*/
			/* byte swap*/
			temperature = ((*data & 0xFF) << 8) |
				      ((*data & 0xFF00) >> 8);
			temperature >>= (8 - mode);

		} else {
			/* insert fractional part and unwanted bits as 0s.*/
			*data <<= (8 - mode);
			temperature = ((*data & 0xFF) << 8) |
				      ((*data & 0xFF00) >> 8);

		}
	}

	*data = temperature;
}
static int tmp102_read_temperature(void *tmp102_dev, int16_t *data)
{
	struct eclite_device *thermal_device = tmp102_dev;
	struct thermal_driver_data *thermal_data = thermal_device->driver_data;
	struct tmp102_interface *dev_interface =
		thermal_device->hw_interface->device;
	int ret;
	int16_t temp = 0;

	ret = eclite_i2c_read_word(dev_interface->i2c_dev,
				   thermal_data->i2c_slave,
				   TMP102_TEMPERATURE_REG, &temp);
	if (ret) {
		LOG_ERR("Temperature read failed. ret: %d", ret);
		return ret;
	}
	tmp102_convert_temperature(&temp, TMP102_READABLE_FORMAT,
				   TMP102_EM_NORAML_MODE);

	*data = temp;
	ECLITE_LOG_DEBUG("Temp: %d", temp);
	return ret;
}


static int tmp102_update_threshold(void *tmp102_dev)
{
	int ret;
	struct eclite_device *thermal_device = tmp102_dev;
	struct thermal_driver_data *thermal_data = thermal_device->driver_data;
	struct tmp102_interface *dev_interface =
		thermal_device->hw_interface->device;

	int16_t temp;

	temp = thermal_data->critical;
	tmp102_convert_temperature(&temp, TMP102_REG_FORMAT,
				   TMP102_EM_NORAML_MODE);
	ret = eclite_i2c_write_word(dev_interface->i2c_dev,
				    thermal_data->i2c_slave,
				    TMP102_T_HIGH_REG, temp);
	if (ret) {
		LOG_ERR("Failed to update high threshold: %d", ret);
		return ret;
	}
	temp = thermal_data->low_threshold;
	tmp102_convert_temperature(&temp, TMP102_REG_FORMAT,
				   TMP102_EM_NORAML_MODE);
	ret = eclite_i2c_write_word(dev_interface->i2c_dev,
				    thermal_data->i2c_slave,
				    TMP102_T_LOW_REG, temp);
	if (ret) {
		LOG_ERR("Failed to update low threshold: %d", ret);
		return ret;
	}

	return 0;
}

static enum device_err_code tmp102_isr(void *tmp102_dev)
{
	struct eclite_device *thermal_device = tmp102_dev;
	struct thermal_driver_data *thermal_data = thermal_device->driver_data;
	int ret;
	int16_t temp = 0;

	ECLITE_LOG_DEBUG("Thermal device %s ISR", thermal_device->name);

	/* isr handling*/
	ret = tmp102_read_temperature(tmp102_dev, &temp);
	if (ret) {
		return ret;
	}
	thermal_data->temperature = temp;
	ECLITE_LOG_DEBUG("Temp: %d\n", temp);
	ret = tmp102_update_threshold(tmp102_dev);
	if (ret) {
		LOG_ERR("Update threshold failed");
		return ret;
	}
	return 0;
}

static enum device_err_code tmp102_init(void *tmp102_dev)
{
	struct eclite_device *thermal_device = tmp102_dev;
	struct thermal_driver_data *thermal_data = thermal_device->driver_data;
	struct tmp102_configuration tmp102_config_data;
	int8_t ret = DEV_INIT_FAILED;
	int16_t temp = 0;
	struct tmp102_interface *dev_interface =
		thermal_device->hw_interface->device;

	ECLITE_LOG_DEBUG("Thermal device %s init", thermal_device->name);
	dev_interface->i2c_dev = (struct device *)
				 eclite_get_i2c_device(ECLITE_I2C_DEV);
	thermal_device->hw_interface->gpio_dev = (struct device *)
			eclite_get_gpio_device(THERMAL_SENSOR_GPIO_NAME);

	tmp102_config_data.extended_mode = TMP102_EM_NORAML_MODE;
	tmp102_config_data.conversion_rate = TMP102_CONVERSION_1_HZ;
	tmp102_config_data.shutdown_mode = TMP102_SD_ACTIVE_MODE;
	tmp102_config_data.thermostat_mode = TMP102_TM_INTERRUPT_MODE;
	tmp102_config_data.pol = TMP102_POL_ACTIVE_LOW;
	tmp102_config_data.fault_queue = TMP102_FX_RETRY_2;

	ret = tmp102_config(tmp102_dev, (uint16_t *)&tmp102_config_data);
	if (ret) {
		LOG_ERR("Sensor config failed");
		return ret;
	}

	ret = tmp102_read_temperature(tmp102_dev, &temp);
	if (ret) {
		LOG_ERR("Temp read failed");
		return ret;
	}
	thermal_data->temperature = temp;
	ECLITE_LOG_DEBUG("Temp: %d", temp);
	thermal_data->high_threshold = THM_HIGH_TEMPERATURE;
	thermal_data->low_threshold = THM_LOW_TEMPERATURE;
	thermal_data->critical = dev_interface->crit_temp;
	tmp102_update_threshold(tmp102_dev);

	if (thermal_data->temperature >= dev_interface->crit_temp) {
		ret = DEV_SHUTDOWN;
	}
	return ret;
}

/* Thermal sensor interface APIs. */
static APP_GLOBAL_VAR(1) struct thermal_driver_api tmp102_api = {
	.read_data = tmp102_read_temperature,
	.configure_data = tmp102_config,
	.status_data = tmp102_get_status,
};

/* Thermal sensor 1.*/
static APP_GLOBAL_VAR_BSS(1) struct thermal_driver_data tmp102_data_1;
/* device private interface */
static APP_GLOBAL_VAR(1) struct tmp102_interface tmp102_device_1 = {
	.crit_temp = TEMP_SYS_CRIT_SHUTDOWN,
};
/* HW interface configuration for battery. */
static APP_GLOBAL_VAR(1) struct hw_configuration tmp102_hw_interface_1 = {
	.device = &tmp102_device_1,
};
APP_GLOBAL_VAR(1) struct eclite_device tmp102_sensor1 = {
	.name = "TMP102_1",
	.device_typ = DEV_THERMAL,
	.instance = INSTANCE_1,
	.init = tmp102_init,
	.driver_api = &tmp102_api,
	.driver_data = &tmp102_data_1,
	.isr = tmp102_isr,
	.hw_interface = &tmp102_hw_interface_1,
};

/* Thermal sensor 2.*/
static APP_GLOBAL_VAR_BSS(1) struct thermal_driver_data tmp102_data_2;
/* device private interface */
static APP_GLOBAL_VAR(1) struct tmp102_interface tmp102_device_2 = {
	.crit_temp = TEMP_SYS_CRIT_SHUTDOWN,
};
/* HW interface configuration for battery. */
static APP_GLOBAL_VAR(1) struct hw_configuration tmp102_hw_interface_2 = {
	.device = &tmp102_device_2,
};
APP_GLOBAL_VAR(1) struct eclite_device tmp102_sensor2 = {
	.name = "TMP102_2",
	.device_typ = DEV_THERMAL,
	.instance = INSTANCE_2,
	.init = tmp102_init,
	.driver_api = &tmp102_api,
	.driver_data = &tmp102_data_2,
	.isr = tmp102_isr,
	.hw_interface = &tmp102_hw_interface_2,
};

/* Thermal sensor 3.*/
static APP_GLOBAL_VAR_BSS(1) struct thermal_driver_data tmp102_data_3;

/* device private interface */
static APP_GLOBAL_VAR(1) struct tmp102_interface tmp102_device_3 = {
	.crit_temp = TEMP_SYS_CRIT_SHUTDOWN,
};
/* HW interface configuration for battery. */
static APP_GLOBAL_VAR(1) struct hw_configuration tmp102_hw_interface_3 = {
	.device = &tmp102_device_3,
};
APP_GLOBAL_VAR(1) struct eclite_device tmp102_sensor3 = {
	.name = "TMP102_3",
	.device_typ = DEV_THERMAL,
	.instance = INSTANCE_3,
	.init = tmp102_init,
	.driver_api = &tmp102_api,
	.driver_data = &tmp102_data_3,
	.isr = tmp102_isr,
	.hw_interface = &tmp102_hw_interface_3,
};
/* Thermal sensor 4.*/
static APP_GLOBAL_VAR_BSS(1) struct thermal_driver_data tmp102_data_4;
/* device private interface */
static APP_GLOBAL_VAR(1) struct tmp102_interface tmp102_device_4 = {
	.crit_temp = TEMP_SYS_CRIT_SHUTDOWN,
};
/* HW interface configuration for battery. */
static APP_GLOBAL_VAR(1) struct hw_configuration tmp102_hw_interface_4 = {
	.device = &tmp102_device_4,
};
APP_GLOBAL_VAR(1) struct eclite_device tmp102_sensor4 = {
	.name = "TMP102_4",
	.device_typ = DEV_THERMAL,
	.instance = INSTANCE_4,
	.init = tmp102_init,
	.driver_api = &tmp102_api,
	.driver_data = &tmp102_data_4,
	.isr = tmp102_isr,
	.hw_interface = &tmp102_hw_interface_4,
};

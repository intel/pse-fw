/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "eclite_device.h"
#include "eclite_hostcomm.h"
#include "thermal_framework.h"
#include "platform.h"
#include "eclite_hw_interface.h"

LOG_MODULE_REGISTER(thermal_framework, CONFIG_ECLITE_LOG_LEVEL);

static APP_GLOBAL_VAR_BSS(1) uint8_t tj_max;
static APP_GLOBAL_VAR(1) bool tj_flag;

static void update_sensor_status(uint16_t *thermal_status,
				 uint16_t thermal_dev_type,
				 uint16_t status)
{
	*thermal_status |= BIT(thermal_dev_type);

	ECLITE_LOG_DEBUG("Sensor[%d] status: %d", thermal_dev_type, status);
}

static void check_thermal_condition(struct eclite_device *thermal_dev,
				    uint16_t *status,
				    uint16_t thermal_dev_type)
{
	struct thermal_driver_data *thermal_data =
		thermal_dev->driver_data;

	if (thermal_dev_type == CPU) {
		if (thermal_data->temperature > thermal_data->critical) {
			update_sensor_status(status,
					     thermal_dev_type,
					     CPU_CRITICAL_THRESHOLD);
		} else if (thermal_data->temperature >
			   thermal_data->high_threshold) {
			update_sensor_status(status,
					     thermal_dev_type,
					     CPU_HIGH_THRESHOLD);
		} else if (thermal_data->temperature <
			   thermal_data->low_threshold) {
			update_sensor_status(status,
					     thermal_dev_type,
					     CPU_LOW_THRESHOLD);
		}
	} else {
		if (thermal_data->temperature > thermal_data->critical) {
			update_sensor_status(status,
					     thermal_dev_type,
					     THERMAL_CRITICAL_THRESHOLD);
		} else if (thermal_data->temperature >
			   thermal_data->high_threshold) {
			update_sensor_status(status,
					     thermal_dev_type,
					     THERMAL_HIGH_THRESHOLD);
		} else if (thermal_data->temperature <
			   thermal_data->low_threshold) {
			update_sensor_status(status,
					     thermal_dev_type,
					     THERMAL_LOW_THRESHOLD);
		}
	}
}

static int process_thermal(struct eclite_device *thermal_dev,
			   uint16_t *status,
			   uint16_t thermal_dev_type)
{
	if (thermal_dev == NULL) {
		LOG_ERR("Thermal sensor device not found");
		return DEV_NOT_FOUND;
	}

	int ret;
	struct thermal_driver_api *thermal_api = thermal_dev->driver_api;
	struct thermal_driver_data *thermal_data =
		thermal_dev->driver_data;

	ret = thermal_api->read_data(thermal_dev,
				     &thermal_data->temperature);

	if (ret) {
		LOG_ERR("Sensor[%d] read failed: %d", thermal_dev_type, ret);
		return ret;
	}
	check_thermal_condition(thermal_dev, status, thermal_dev_type);
	/* Update temperature threshold for CPU for 3 degree delta alert*/
	if (thermal_dev_type == CPU) {
		if (*status & BIT(CPU)) {
			thermal_data->high_threshold =
				thermal_data->temperature + CPU_TEMP_DELTA;
			thermal_data->low_threshold =
				thermal_data->temperature - CPU_TEMP_DELTA;
		}
	}
	return 0;
}

static int control_fan(struct eclite_device *fan)
{
	if (!fan) {
		LOG_ERR("Fan device not found");
		return DEV_NOT_FOUND;
	}

	int ret;
	struct thermal_driver_api *fan_api = fan->driver_api;
	struct thermal_driver_data *fan_data = fan->driver_data;

	ret = fan_api->write_data(fan, &fan_data->rotation);
	if (ret) {
		LOG_ERR("Fan configure failed: %d", ret);
	}
	return 0;
}

static int read_tacho(struct eclite_device *tacho)
{
	if (!tacho) {
		LOG_ERR("Tacho device not found");
		return DEV_NOT_FOUND;
	}

	int ret;
	struct thermal_driver_api *tacho_api = tacho->driver_api;
	struct thermal_driver_data *tacho_data = tacho->driver_data;

	ret = tacho_api->read_data(tacho, &tacho_data->tacho);
	if (ret) {
		LOG_WRN("Tacho read failed: %d", ret);
		return ret;
	}
	return 0;
}

int thermal_callback(uint16_t *status)
{
	struct eclite_device *thermal;
	struct eclite_device *cpu = find_dev_by_type(DEV_THERMAL_CPU);
	struct eclite_device *tacho = find_dev_by_type(DEV_FAN);
	struct eclite_device *fan = find_dev_by_type(DEV_FAN);
	int ret = 0;

	/* reset framework status.*/
	*status = 0;

	ECLITE_LOG_DEBUG("Executing thermal framework");

	if (!tj_flag) {
		int ret;

		ret = pmc_command(GET_TJ_MAX, &tj_max);
		if (ret) {
			LOG_ERR("TJMAX request to PMC failed: %d", ret);
		} else {
			tj_flag = true;
		}
	}

	/* run over all possible instance of thermistor */
	for (int i = 0; i < MAX_THERMAL_SENSOR; i++) {
		thermal = find_dev_instance_by_type(DEV_THERMAL, i + 1);
		/* process on board thermal sensors */
		ret |= process_thermal(thermal, status, i);
	}
	/* Read CPU temperature */
	ret |= process_thermal(cpu, status, CPU);
	if (!thermal_disable_d0ix) {
		/* Control FAN */
		ret |= control_fan(fan);
		/* Read Tacho */
		ret |= read_tacho(tacho);
	} else {
		ECLITE_LOG_DEBUG("D0ix entry Fan Disabled");
	}

	if (ret) {
		LOG_WRN("Thermal callback: %d", ret);
		return ret;
	}

	return 0;
}

void thermal_command(void *command)
{
	struct thermal_command_data *msg = command;
	struct eclite_device *fan = find_dev_by_type(DEV_FAN);

	/* Look for fan device, if not found return error */
	if (!fan) {
		LOG_ERR("No fan device");
		return;
	}

	struct thermal_driver_data *fan_data = fan->driver_data;
	struct thermal_driver_api *fan_api = fan->driver_api;
	struct eclite_device *thermal;

	switch (msg->event_id) {
	case ECLITE_OS_EVENT_PWM_UPDATE:
		fan_data->rotation = msg->data[0];
		fan_api->write_data(fan, &fan_data->rotation);
		break;
	case ECLITE_OS_EVENT_THERM_ALERT_TH_UPDATE:
		for (int i = 0; i < MAX_THERMAL_SENSOR; i++) {
			thermal = find_dev_instance_by_type(DEV_THERMAL, i + 1);
			if (!thermal) {
				continue;
			}
			struct thermal_driver_data *data =
				thermal->driver_data;
			if (!data) {
				continue;
			}
			data->high_threshold = msg->data[2 * i + 0];
			data->low_threshold = msg->data[2 * i + 1];
		}
		break;
	case ECLITE_OS_EVENT_CRIT_TEMP_UPDATE:
		for (int i = 0; i < MAX_THERMAL_SENSOR; i++) {
			thermal = find_dev_instance_by_type(DEV_THERMAL, i + 1);
			if (!thermal) {
				continue;
			}
			struct thermal_driver_data *data =
				thermal->driver_data;
			if (!data) {
				continue;
			}
			data->critical = msg->data[i];
		}
		break;
	default:
		LOG_ERR("Unknown thermal command: %d", msg->event_id);
		break;
	}
}

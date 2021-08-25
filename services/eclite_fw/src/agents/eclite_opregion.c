/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain/gcc.h>

#include "eclite_opregion.h"
#include "eclite_device.h"
#include "charger_framework.h"
#include "eclite_hostcomm.h"
#include "thermal_framework.h"
#include "common.h"

LOG_MODULE_REGISTER(opregion, CONFIG_ECLITE_LOG_LEVEL);

static void update_charger_region(void)
{
	struct eclite_device *charger_dev = find_dev_by_type(DEV_CHG);

	/* Look for charger device, if not found return error */
	if (!charger_dev) {
		LOG_ERR("No charger device");
		return;
	}

	struct charger_driver_data *charger_data = charger_dev->driver_data;

	eclite_opregion.psrc = charger_data->ac_present & BIT0;
	ECLITE_LOG_DEBUG("psrc: %d, ac_present: %d",
			 eclite_opregion.psrc,
			 charger_data->ac_present);
}
static void update_battery_region(void)
{
	struct eclite_device *charger_dev = find_dev_by_type(DEV_CHG);

	/* Look for charger device, if not found return error */
	if (!charger_dev) {
		LOG_ERR("No charger device");
		return;
	}

	struct eclite_device *sbs_dev = find_dev_by_type(DEV_FG);

	/* Look for sbs_dev device, if not found return error */
	if (!sbs_dev) {
		LOG_ERR("No fuel gauge device");
		return;
	}

	struct sbs_driver_data *battery_data = sbs_dev->driver_data;
	const struct sbs_driver_api *battery_api = sbs_dev->driver_api;

	const struct charger_driver_api *charger_api = charger_dev->driver_api;

	uint8_t ret = 0;

	ARG_UNUSED(charger_dev);
	ARG_UNUSED(charger_api);
	eclite_opregion.battery_info.state =
		battery_data->chargering_status;

	ret = battery_api->voltage(
		sbs_dev,
		&eclite_opregion.battery_info.battery_voltage);

	ret |= battery_api->at_rate(sbs_dev,
				    &eclite_opregion.battery_info.preset_rate);

	ret |= battery_api->remaining_capacity(
		sbs_dev,
		&eclite_opregion.battery_info.remaining_capacity);

	ret |= battery_api->design_capacity(
		sbs_dev,
		&eclite_opregion.battery_info.design_capacity);

	ret |= battery_api->full_charge_capacity(
		sbs_dev,
		&eclite_opregion.battery_info.full_charge_capacity);

	ret |= battery_api->design_voltage(
		sbs_dev,
		&eclite_opregion.battery_info.design_voltage);

	ret |= battery_api->cycle_count(
		sbs_dev,
		&eclite_opregion.battery_info.cycle_count);

	/* discharge_rate */
	if ((int16_t)eclite_opregion.battery_info.preset_rate < 0) {
		eclite_opregion.battery_info.discharge_rate =
			eclite_opregion.battery_info.preset_rate;
	}

	if (ret) {
		LOG_WRN("Error-%d updating opregion", ret);
	}
}

static void update_fan_region(void)
{
	struct eclite_device *fan = find_dev_by_type(DEV_FAN);

	/* Look for fan device, if not found return error */
	if (!fan) {
		LOG_ERR("No fan device");
		return;
	}

	struct thermal_driver_data *fan_data = fan->driver_data;

	eclite_opregion.tacho_rpm = fan_data->tacho;
}

static void update_thermal_region(void)
{
	struct eclite_device *thermal;
	struct thermal_driver_data *thermal_data;
	uint16_t *opregion_thermal;

	opregion_thermal = &eclite_opregion.systherm0_temp1;

	/* run over all possible instance of thermistor */
	for (int i = 0; i < MAX_THERMAL_SENSOR; i++) {

		thermal = find_dev_instance_by_type(DEV_THERMAL, i + 1);
		if (!thermal) {
			continue;
		}
		thermal_data = thermal->driver_data;
		if (!thermal_data) {
			continue;
		}

		/* process on board thermal sensors */
		*opregion_thermal = thermal_data->temperature;

		/* skip word to point to correct offset. */
		opregion_thermal += 1;
	}

}

static void initialize_thermal_region(void)
{
	struct eclite_device *thermal;
	struct thermal_driver_data *thermal_data;
	uint16_t *opregion_thermal_alert;
	uint16_t *opregion_thermal_critical_temp;

	opregion_thermal_alert = &eclite_opregion.sys0_alert3_1;
	opregion_thermal_critical_temp = &eclite_opregion.sys0_crittemp_thr1;

	/* run over all possible instance of thermistor */
	for (int i = 0; i < MAX_THERMAL_SENSOR; i++) {
		thermal = find_dev_instance_by_type(DEV_THERMAL, i + 1);
		if (!thermal) {
			continue;
		}
		thermal_data = thermal->driver_data;
		if (!thermal_data) {
			continue;
		}

		/* initialize on board thermal sensors alert */
		*opregion_thermal_alert = thermal_data->high_threshold;
		opregion_thermal_alert += 1;
		*opregion_thermal_alert = thermal_data->low_threshold;
		*opregion_thermal_critical_temp = thermal_data->critical;

		/* skip word to point to correct offset */
		opregion_thermal_alert += 1;
		opregion_thermal_critical_temp += 1;
	}
}

static void update_thermal_cpu_region(void)
{
	struct eclite_device *cpu_dev;
	struct thermal_driver_data *cpu_data;
	uint16_t *opregion_cpu;

	opregion_cpu = &eclite_opregion.cpu_temperature;
	cpu_dev = find_dev_by_type(DEV_THERMAL_CPU);

	if (!cpu_dev) {
		*opregion_cpu = 0;
		LOG_ERR("No CPU thermal device");
		return;
	}

	cpu_data = cpu_dev->driver_data;
	if (!cpu_data) {
		*opregion_cpu = 0;
		LOG_ERR("No CPU thermal device data");
		return;
	}

	/* Update CPU temperature */
	*opregion_cpu = cpu_data->temperature;
}

static void initialize_thermal_cpu_region(void)
{
	struct eclite_device *cpu_dev;
	struct thermal_driver_data *cpu_data;
	uint16_t *opregion_cpu;

	opregion_cpu = &eclite_opregion.cpu_crittemp_thr;
	cpu_dev = find_dev_by_type(DEV_THERMAL_CPU);

	if (!cpu_dev) {
		*opregion_cpu = 0;
		LOG_ERR("No CPU thermal device");
		return;
	}

	cpu_data = cpu_dev->driver_data;
	if (!cpu_data) {
		*opregion_cpu = 0;
		LOG_ERR("No CPU thermal device data");
		return;
	}

	/* Update CPU default critical threshold */
	*opregion_cpu = cpu_data->critical;
}

static void initialize_ucsi_region(void)
{
	uint16_t *opregion_ucsi_ver;

	opregion_ucsi_ver = &eclite_opregion.ucsi_in_data.ucsi_version;

	/* Update ucsi version in opregion */
	*opregion_ucsi_ver = UCSI_VERSION_STR;
}

void update_opregion(uint8_t device)
{
	ECLITE_LOG_DEBUG("Device - %d\n", device);

	switch (device) {
	case DEV_CHG:
	case DEV_FG:
		update_charger_region();
		update_battery_region();
		break;
	case DEV_FAN:
		update_fan_region();
		break;
	case DEV_THERMAL:
		update_thermal_region();
		break;
	case DEV_THERMAL_CPU:
		update_thermal_cpu_region();
		break;
	case DEV_UCSI:
		break;
	default:
		break;
	}
}

void initialize_opregion(uint8_t device)
{
	ECLITE_LOG_DEBUG("Device - %d\n", device);

	switch (device) {
	case DEV_CHG:
	case DEV_FG:
		break;
	case DEV_FAN:
		break;
	case DEV_THERMAL:
		initialize_thermal_region();
		break;
	case DEV_THERMAL_CPU:
		initialize_thermal_cpu_region();
		break;
	case DEV_UCSI:
		initialize_ucsi_region();
		break;
	default:
		break;
	}
}

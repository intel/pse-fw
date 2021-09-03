/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "eclite_device.h"
#include "eclite_hw_interface.h"
#include "thermal_framework.h"
#include "platform.h"
#include "cpu.h"
#include <sedi.h>

LOG_MODULE_REGISTER(cpu, CONFIG_ECLITE_LOG_LEVEL);

struct cpu_interface {
	uint16_t crit_temp;
};

static int get_cpu_temperature(void *cpu, int16_t *data)
{
	int ret;

	if (eclite_sx_state != PM_RESET_TYPE_S0) {
		LOG_ERR("%s() in non-S0\n", __func__);
		return ERROR;
	}
	if (eclite_s0ix_state) {
		LOG_ERR("%s() in S0ix\n", __func__);
		return ERROR;
	}

	ret = pmc_command(GET_TEMP, (uint8_t *)data);

	if (ret) {
		LOG_ERR("Get temp failed: %d", ret);
		return ret;
	}

	return 0;
}

static enum device_err_code cpu_init(void *cpu)
{
	struct eclite_device *cpu_dev = cpu;
	struct thermal_driver_data *cpu_data =
		cpu_dev->driver_data;

	cpu_data->high_threshold = CPU_HIGH_TEMPERATURE;
	cpu_data->low_threshold = CPU_LOW_TEMPERATURE;
	cpu_data->critical = TEMP_CPU_CRIT_SHUTDOWN;

	return 0;
}

static enum device_err_code cpu_isr(void *cpu)
{
	/*ToDO: hook for future need.*/
	return 0;
}

/* CPU interface APIs. */
static APP_GLOBAL_VAR(1) struct thermal_driver_api cpu_api = {
	.read_data = get_cpu_temperature,
};

/* CPU - thermal reading.*/
static APP_GLOBAL_VAR_BSS(1) struct thermal_driver_data cpu_data;
/* device private interface */
static APP_GLOBAL_VAR(1) struct cpu_interface cpu_internal = {
	.crit_temp = TEMP_CPU_CRIT_SHUTDOWN,
};
/* HW interface configuration for CPU. */
static APP_GLOBAL_VAR(1) struct hw_configuration cpu_hw_interface = {
	.device = &cpu_internal,
};

APP_GLOBAL_VAR(1) struct eclite_device thermal_cpu = {
	.name = "DEV_THERMAL_CPU",
	.device_typ = DEV_THERMAL_CPU,
	.instance = INSTANCE_1,
	.init = cpu_init,
	.driver_api = &cpu_api,
	.driver_data = &cpu_data,
	.isr = cpu_isr,
	.hw_interface = &cpu_hw_interface,
};

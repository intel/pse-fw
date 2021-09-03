/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <toolchain/gcc.h>
#include "charger_framework.h"
#include "eclite_device.h"
#include "common.h"
#include "platform.h"
#include "eclite_hw_interface.h"
#include "eclite_dispatcher.h"
#include "eclite_hostcomm.h"

LOG_MODULE_REGISTER(charger_framework, CONFIG_ECLITE_LOG_LEVEL);

static APP_GLOBAL_VAR(1) uint8_t charger_present_old;
static APP_GLOBAL_VAR(1) uint8_t charge_percentage;

static void cm_enable_charging(struct eclite_device *charger_dev)
{
	const struct charger_driver_api *drv_api = charger_dev->driver_api;

	drv_api->enable_charging(charger_dev);
}

static void cm_disable_charging(struct eclite_device *charger_dev)
{
	const struct charger_driver_api *drv_api = charger_dev->driver_api;

	drv_api->disable_charging(charger_dev);
}

static void update_charger(struct eclite_device *charger_dev,
			   struct eclite_device *sbs_dev)
{
	/* Reference API created for I2C based charger control */
}

static int update_battery_trip_point(struct eclite_device *sbs_dev)
{
	struct sbs_driver_api *battery_drv_api = sbs_dev->driver_api;
	struct sbs_driver_data *battery_drv_data = sbs_dev->driver_data;

	return battery_drv_api->battery_trip_point(
		       sbs_dev, &battery_drv_data->sbs_battery_trip_point);
}

static void update_chager_status(struct eclite_device *charger_dev,
				 struct eclite_device *battery_dev,
				 uint8_t charge_status)
{
	struct charger_driver_data *charger_drv_data =
		charger_dev->driver_data;
	struct sbs_driver_data *battery_drv_data = battery_dev->driver_data;

	ARG_UNUSED(charger_drv_data);

	switch (charge_status) {
	case BATTERY_CHARGING:
		/* AC & battery present */
		battery_drv_data->chargering_status &= ~BATTERY_DISCHARGING;
		battery_drv_data->chargering_status |= BATTERY_CHARGING;
		battery_drv_data->chargering_status |= BATTERY_PRESENT;
		battery_drv_data->chargering_status &= ~BATTERY_CRITICAL;
		break;

	case BATTERY_DISCHARGING:
		battery_drv_data->chargering_status &= ~BATTERY_CHARGING;
		battery_drv_data->chargering_status |= BATTERY_DISCHARGING;
		battery_drv_data->chargering_status |= BATTERY_PRESENT;
		battery_drv_data->chargering_status &= ~BATTERY_CRITICAL;
		break;

	case AC_ONLY:
		/* only AC present */
		battery_drv_data->chargering_status &= ~BATTERY_CHARGING;
		battery_drv_data->chargering_status &= ~BATTERY_DISCHARGING;
		battery_drv_data->chargering_status &= ~BATTERY_PRESENT;
		battery_drv_data->chargering_status &= ~BATTERY_CRITICAL;
		break;
	case BATTERY_FULL:
		/* only AC present */
		battery_drv_data->chargering_status &= ~BATTERY_CHARGING;
		battery_drv_data->chargering_status &= ~BATTERY_DISCHARGING;
		battery_drv_data->chargering_status |= BATTERY_PRESENT;
		battery_drv_data->chargering_status &= ~BATTERY_CRITICAL;
		break;
	default:
		battery_drv_data->chargering_status &= ~BATTERY_CHARGING;
		battery_drv_data->chargering_status &= ~BATTERY_DISCHARGING;
		battery_drv_data->chargering_status &= ~BATTERY_PRESENT;
		battery_drv_data->chargering_status |= BATTERY_CRITICAL;
		break;
	}
}

static void charger_flow(struct eclite_device *charger_dev,
			 struct eclite_device *sbs_dev,
			 uint16_t absolute_state_of_charge)
{
	struct charger_driver_data *charger_data = charger_dev->driver_data;
	struct sbs_driver_data *sbs_data = sbs_dev->driver_data;

	ARG_UNUSED(sbs_data);

	if (charger_data->ac_present) {
		if (absolute_state_of_charge >= SOC_FULL_BAT_CAP) {
			cm_disable_charging(charger_dev);
			update_chager_status(charger_dev,
					     sbs_dev,
					     BATTERY_FULL);
		} else if ((absolute_state_of_charge <
			    SOC_BAT_MAINTAINCE_THRS)
			   && (sbs_data->chargering_status
			       & BATTERY_CHARGING)) {
			cm_enable_charging(charger_dev);
			update_chager_status(charger_dev, sbs_dev,
					     BATTERY_CHARGING);

		} else {
			cm_enable_charging(charger_dev);
			update_chager_status(charger_dev, sbs_dev,
					     BATTERY_CHARGING);

		}
	}
}

int charging_manager_callback(uint8_t event, uint16_t *status)
{
	struct eclite_device *charger_dev = find_dev_by_type(DEV_CHG);

	/* Look for charger device, if not found return error */
	if (!charger_dev) {
		LOG_ERR("No charger device");
		return ERROR;
	}

	struct eclite_device *sbs_dev = find_dev_by_type(DEV_FG);

	/* Look for fuel gage device, if not found return error */
	if (!sbs_dev) {
		LOG_ERR("No fuel gauge device");
		return ERROR;
	}

	struct sbs_driver_data *battery_data = sbs_dev->driver_data;
	struct charger_driver_data *charger_data = charger_dev->driver_data;

	const struct charger_driver_api *charger_api = charger_dev->driver_api;
	const struct sbs_driver_api *battery_api = sbs_dev->driver_api;

	int charger_present;
	int bat_present;
	uint8_t power_device;
	uint16_t btp = 0x0;
	int ret;
	uint16_t battery_status = 0, absolute_charge = 0, charger_status = 0;
	int battery_warning = 0;

	ARG_UNUSED(battery_data);

	ECLITE_LOG_DEBUG("Executing charging framework");

	bat_present = battery_api->check_battery_presence(sbs_dev) ? 0 : 1;
	ret = charger_api->check_ac_present(charger_dev);
	if (ret) {
		LOG_ERR("Error detecting charger presence");
	}

	charger_present = charger_data->ac_present;

	if (bat_present) {
		struct device *gpio_dev = (struct device *)
			charger_dev->hw_interface->gpio_dev;
		struct platform_gpio_config *cfg =
			(struct platform_gpio_config *)
			(charger_dev->hw_interface->gpio_config);
		uint32_t pin_value;

		pin_value = gpio_pin_get_raw(gpio_dev, CHARGER_GPIO);
		if (pin_value) {
			cfg->gpio_config.intr_type &= ~(GPIO_ACTIVE_HIGH);
		} else {
			cfg->gpio_config.intr_type |= (GPIO_ACTIVE_HIGH);
		}
		ret = eclite_gpio_configure(gpio_dev, CHARGER_GPIO,
					    cfg->gpio_config.dir |
					    cfg->gpio_config.pull_down_en |
					    cfg->gpio_config.intr_type);
		if (ret) {
			LOG_ERR("GPIO: %u configure err", CHARGER_GPIO);
				return ret;
		}
	}

	*status |= charger_present << 0;
	*status |= ((charger_present ^ charger_present_old) << 1);
	charger_present_old = charger_present;

	battery_warning = battery_api->battery_status(sbs_dev, &battery_status);
	battery_api->absolute_state_of_charge(sbs_dev, &absolute_charge);

	if (absolute_charge != charge_percentage) {
		*status |= BIT2;
		charge_percentage = absolute_charge;
	} else {
		*status &= ~BIT2;
	}

	charger_api->get_charger_status(charger_dev, &charger_status);

	/* process events */
	switch (event) {
	case FG_EVENT:
		ECLITE_LOG_DEBUG("%s(): Received interrupt from FG! Update BTP",
				 __func__);
		battery_api->battery_trip_point(sbs_dev, &btp);
		break;
	case CHG_EVENT:
		/* Charger Event */
		break;
	default:
		LOG_WRN("Invalid charger event");
		ret = ERROR;
		break;
	}

	/* check connected power sources */
	power_device = charger_present | bat_present << 1;

	switch (power_device) {
	case CHARGER_ONLY:
		ECLITE_LOG_DEBUG("Power Supply: Charger Only");
		update_chager_status(charger_dev, sbs_dev, AC_ONLY);
		break;
	case BATTERY_ONLY:
		ECLITE_LOG_DEBUG("Power Supply: Battery Only");
		cm_disable_charging(charger_dev);
		update_chager_status(charger_dev, sbs_dev, BATTERY_DISCHARGING);
		break;
	case CHARGER_BATTERY:
		ECLITE_LOG_DEBUG("Power Supply: Charger & Battery");
		update_charger(charger_dev, sbs_dev);
		charger_flow(charger_dev, sbs_dev, absolute_charge);
		break;
	default:
		LOG_WRN("Invalid power source");
		ret = ERROR;
		break;
	}


	/* handle critical errors */
	if (battery_warning && bat_present) {
		cm_disable_charging(charger_dev);
		update_chager_status(charger_dev,
				     sbs_dev,
				     BATTERY_DISCHARGING);

		LOG_ERR("Charging framework critical error");
		ret = ERROR;
	}

	return ret;
}

void charger_framework_command(void *command)
{
	struct charger_framework_data *msg = command;
	struct eclite_device *sbs_dev = find_dev_by_type(DEV_FG);

	/* Look for fuel gage device, if not found return error */
	if (!sbs_dev) {
		LOG_ERR("No fuel gauge device");
		return;
	}

	struct sbs_driver_data *battery_data = sbs_dev->driver_data;

	switch (msg->event_id) {
	case ECLITE_OS_EVENT_BTP_UPDATE:
		battery_data->sbs_battery_trip_point = msg->data;
		update_battery_trip_point(sbs_dev);
		break;
	default:
		ECLITE_LOG_DEBUG("Charger command: %d", msg->event_id);

	}
}

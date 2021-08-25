/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief This file exposes interface to access charger manager.
 */

#ifndef _CHARGER_FRAMEWORK_H_
#define _CHARGER_FRAMEWORK_H


#include <stdint.h>
#include <logging/log.h>
#include "eclite_hw_interface.h"

/**
 * @cond INTERNAL_HIDDEN
 *
 * This defines are used by charging manager internally.
 */

enum powerdevice_t {
	CHARGER_ONLY = 1,
	BATTERY_ONLY,
	CHARGER_BATTERY,
};

#define SOC_FULL_BAT_CAP        100
#define SOC_LOW_BAT_CAP         5
#define SOC_BAT_BTP             10
#define SOC_BAT_MAINTAINCE_THRS 95

/* Battery status as per ACPI requirement */
#define BATTERY_DISCHARGING     BIT0
#define BATTERY_CHARGING        BIT1
#define BATTERY_CRITICAL        BIT2
#define BATTERY_PRESENT         BIT3
#define AC_ONLY                 BIT4
#define BATTERY_FULL            BIT5

/** @brief Structure to process charger framework commands.
 *
 *  Structure is used to update parameter sent by host via opregion
 */
struct charger_framework_data {
	/** command message id. This value is update to opregion*/
	uint8_t event_id;
	/** command parameter to receive parameter from HOST.*/
	uint16_t data;
};

/**
 * @endcond
 */


/** @brief Charger driver data structure.
 *
 *  Structure contains parameter for Charger operation.
 */
struct charger_driver_data {
	/** I2C bus number.*/
	uint16_t i2c_bus_num;
	/** I2C bus slave address for charger.*/
	uint16_t i2c_slave_addr;

	/** charger presence.*/
	uint32_t ac_present;
	/** fuel gauge even.*/
	uint8_t fg_event;

	/** charger parameter voltage.*/
	uint8_t volt;
	/** charger parameter current.*/
	uint8_t curr;
	/** charger parameter inlimit current.*/
	uint8_t inlim;

	/** charger initialization indicator.*/
	uint8_t charger_init;
	/** charger status.*/
	uint8_t charger_status;
	/** charger configuration.*/
	uint8_t charger_config;
	/** charger fault status.*/
	uint8_t charger_fault_status;
	/** charger control parameter.*/
	uint8_t en_charger;
	/** charging control parameter.*/
	uint8_t en_charging;
};

/** @brief This is smart battery driver data.
 *
 *  Structure contains parameter for Charger operation.
 */
struct sbs_driver_data {
	/** I2C bus number for battery.*/
	uint16_t sbs_i2c_bus_num;
	/** I2C slave address for battery.*/
	uint16_t sbs_i2c_slave_addr;

	/** battery charging status.*/
	uint8_t chargering_status;
	/** battery trip point value received from OS.*/
	uint16_t sbs_battery_trip_point;
};


/*
 * Charger Function
 */

/**
 * @brief This function is optional and its meaning is implementation specific.
 *
 * This function is used by charger driver. Other charger APIs has same
 * prototype if not defined explicitly.
 *
 *************@param dev is eclite device pointer for charger.
 *
 *************@retval GPIO driver error codes.
 */
typedef int (*charger_read_voltage_t)(void *dev);

/**
 * @cond INTERNAL_HIDDEN
 *
 * This defines are used by charging manager internally to access fuel gauge.
 */

typedef int (*charger_read_current_t)(void *dev);
typedef int (*charger_read_inlim_t)(void *dev);

typedef int (*charger_write_voltage_t)(void *dev);
typedef int (*charger_write_current_t)(void *dev);
typedef int (*charger_write_inlim_t)(void *dev);

typedef int (*charger_write_inlim_t)(void *dev);

typedef int (*charger_charger_init_t)(void *dev);
typedef int (*charger_charger_status_t)(void *dev);
typedef int (*charger_config_charger_t)(void *dev);
typedef int (*charger_charger_fault_status_t)(void *dev);

typedef int (*charger_en_charger_t)(void *dev);
typedef int (*charger_en_charging_t)(void *dev);
typedef int (*charger_discharging_t)(void *dev);

typedef int (*charger_check_ac_present_t)(void *dev);

/**
 * @endcond
 */

/**
 * @brief This function is optional and its meaning is implementation specific.
 *
 * This function is supported by the smart battery specification 1.1.Other
 * function of the of the smart battery specification is using same function
 * prototype until specified here.
 *
 *************@param dev is eclite device pointer for battery.
 *************@param data is content read from battery.
 *
 *************@retval I2C driver error codes.
 */
typedef int (*manufacturer_access_t)(void *dev, uint16_t *data);

/**
 * @cond INTERNAL_HIDDEN
 *
 * This defines are used by charging manager internally to access fuel gauge.
 */

typedef int (*remaining_capacityalarm_t)(void *dev, uint16_t *data);
typedef int (*remaining_time_alarm_t)(void *dev, uint16_t *data);
typedef int (*battery_mode_t)(void *dev, uint16_t *data);
typedef int (*at_rate_t)(void *dev, uint16_t *data);
typedef int (*at_rate_time_to_full_t)(void *dev, uint16_t *data);
typedef int (*at_rate_time_to_empty_t)(void *dev, uint16_t *data);
typedef int (*at_rate_ok_t)(void *dev, uint16_t *data);
typedef int (*temperature_t)(void *dev, uint16_t *data);
typedef int (*voltage_t)(void *dev, uint16_t *data);
typedef int (*current_t)(void *dev, uint16_t *data);
typedef int (*average_current_t)(void *dev, uint16_t *data);
typedef int (*max_error_t)(void *dev, uint16_t *data);
typedef int (*relative_state_of_charge_t)(void *dev, uint16_t *data);
typedef int (*absolute_state_of_charge_t)(void *dev, uint16_t *data);
typedef int (*remaining_capacity_t)(void *dev, uint16_t *data);
typedef int (*full_charge_capacity_t)(void *dev, uint16_t *data);
typedef int (*run_time_to_empty_t)(void *dev, uint16_t *data);
typedef int (*average_time_to_empty_t)(void *dev, uint16_t *data);
typedef int (*average_time_to_full_t)(void *dev, uint16_t *data);
typedef int (*charging_current_t)(void *dev, uint16_t *data);
typedef int (*charging_voltage_t)(void *dev, uint16_t *data);
typedef int (*battery_status_t)(void *dev, uint16_t *data);
typedef int (*cycle_count_t)(void *dev, uint16_t *data);
typedef int (*design_capacity_t)(void *dev, uint16_t *data);
typedef int (*design_voltage_t)(void *dev, uint16_t *data);
typedef int (*specification_info_t)(void *dev, uint16_t *data);
typedef int (*manufacture_date_t)(void *dev, uint16_t *data);
typedef int (*serial_number_t)(void *dev, uint16_t *data);
typedef int (*manufacturer_name_t)(void *dev, uint16_t *data);
typedef int (*device_name_t)(void *dev, uint16_t *data);
typedef int (*device_chemistry_t)(void *dev, uint16_t *data);
typedef int (*manufacturer_data_t)(void *dev, uint16_t *data);
typedef int (*battery_trip_point_t)(void *dev, uint16_t *data);
/**
 * @endcond
 */

/**
 * @brief This function is used to detect battery presence.
 *
 *************@param dev is eclite device pointer for battery.
 *
 *************@retval I2C driver error codes.
 */
typedef int (*check_battery_presence_t)(void *dev);

/**
 * @brief This function is optional and its meaning is implementation specific.
 *
 * This function is supported by the smart battery specification 1.1.Other
 * function of the of the smart battery specification is using same function
 * prototype until specified here.
 *
 *************@param dev is eclite device pointer for charger.
 *************@param data is charger status.
 *
 *************@retval I2C driver error codes.
 */
typedef int (*get_charger_status_t)(void *dev, uint16_t *data);

/** @brief Charger driver APIs
 *
 *  These APIs are used to communicate and control charger.
 */
struct charger_driver_api {
	/** get charging voltage.*/
	charger_read_voltage_t charger_read_volt;
	/** get charging current.*/
	charger_read_current_t charger_read_curr;
	/** get charging inlimit current.*/
	charger_read_inlim_t charger_read_inlim;
	/** set charging voltage.*/
	charger_write_voltage_t charger_write_volt;
	/** set charging current.*/
	charger_write_current_t charger_write_curr;
	/** set charging inlimit current.*/
	charger_write_inlim_t charger_write_inlim;
	/** charger initialization.*/
	charger_charger_init_t charger_init;
	/** charger status.*/
	charger_charger_status_t charger_status;
	/** charger configuration.*/
	charger_config_charger_t charger_config;
	/** get charger fault status.*/
	charger_charger_fault_status_t charger_fault_status;
	/** control charger.*/
	charger_en_charger_t en_charger;
	/** enable charging.*/
	charger_en_charging_t enable_charging;
	/** disable charging.*/
	charger_discharging_t disable_charging;
	/** check charger presence.*/
	charger_check_ac_present_t check_ac_present;
	/** charger status.*/
	get_charger_status_t get_charger_status;
};

/** @brief Charger driver APIs
 *
 * These APIs are as per smart battery specification 1.1, It is used to talk
 * to fuel gauge.
 */
struct sbs_driver_api {
	/* sbs 1.1 api*/
	/** Returns manufacture access.*/
	manufacturer_access_t manufacturer_access;
	/** Returns battery remaining capacity.*/
	remaining_capacityalarm_t remaining_capacityalarm;
	/** Returns battery remaining time to fullt charge.*/
	remaining_time_alarm_t remainingime_alarm;
	/** Returns battery mode.*/
	battery_mode_t battery_mode;
	/** Returns battery charging rate.*/
	at_rate_t at_rate;
	/** Returns battery time to reach to reach full charge.*/
	at_rate_time_to_full_t at_rateimeo_full;
	/** Returns battery time to reach to empty.*/
	at_rate_time_to_empty_t at_rateimeo_empty;
	/** Indicates whether or not the battery can deliver the previously
	 * written AtRate value
	 */
	at_rate_ok_t at_rate_ok;
	/** Get battery temperature.*/
	temperature_t temperature;
	/** Get battery voltage.*/
	voltage_t voltage;
	/** Get battery current.*/
	current_t current;
	/** Get battery average current.*/
	average_current_t average_current;
	/** Returns expected margin of error in the state of charge calculation.
	 */
	max_error_t max_error;
	/** Returns relative state of charge.*/
	relative_state_of_charge_t relative_state_of_charge;
	/** Returns absolute state of charge.*/
	absolute_state_of_charge_t absolute_state_of_charge;
	/** Returns battery remaining capacity.*/
	remaining_capacity_t remaining_capacity;
	/** Returns the predicted pack capacity when it is fully charged.*/
	full_charge_capacity_t full_charge_capacity;
	/** Returns the predicted remaining battery life at the present rate of
	 * discharge (minutes)
	 */
	run_time_to_empty_t runimeo_empty;
	/**Returns a one minute rolling average of the predicted remaining
	 * battery life.
	 */
	average_time_to_empty_t averageimeo_empty;
	/** Returns a one minute rolling average of the predicted remaining time
	 * until the Smart Battery reaches full charge.
	 */
	average_time_to_full_t averageimeo_full;
	/** Receives the desired charging rate to the Smart Battery Charger.*/
	charging_current_t charging_current;
	/** Receives the desired charging voltage to the Smart Battery Charger
	 */
	charging_voltage_t charging_voltage;
	/** Returns the Smart Battery's status word which contains Alarm and
	 * Status bit flags.
	 */
	battery_status_t battery_status;
	/** Returns the number of cycles the battery has experienced.*/
	cycle_count_t cycle_count;
	/** Returns the theoretical capacity of a new pack.*/
	design_capacity_t design_capacity;
	/** Returns the theoretical voltage of a new pack (mV).*/
	design_voltage_t design_voltage;
	/** Returns the version number of the Smart Battery specification the
	 * battery pack supports, as well as voltage and current and capacity
	 * scaling information in a packed unsigned integer.
	 */
	specification_info_t specification_info;
	/** This function returns the date the cell pack was manufactured in a
	 * packed integer.
	 */
	manufacture_date_t manufacture_date;
	/** This function is used to return a serial number.*/
	serial_number_t serial_number;
	/** This function returns a character array containing the battery's
	 * manufacturer's name.
	 */
	manufacturer_name_t manufacturer_name;
	/** This function returns a character string that contains the battery's
	 * name.
	 */
	device_name_t device_name;
	/** This function returns a character string that contains the battery's
	 * chemistry.
	 */
	device_chemistry_t device_chemistry;
	/** This function allows access to the manufacturer data contained in
	 * the battery.
	 */
	manufacturer_data_t manufacturer_data;
	/** This function updates battery trip point information into battery
	 * which is received from battery.
	 */
	battery_trip_point_t battery_trip_point;
	/** This function detects battery presence.*/
	check_battery_presence_t check_battery_presence;
};

/**
 * @brief This function manages charging flow.
 *
 * @param  event is BATTERY/CHARGER event id.
 * @param  status is to indicate charger status(BIT0) and charger status change
 * (BIT1).
 * @retval SUCCESS if charger and battery status is ok.
 *	   FAILURE if charger and battery status face critical condition.
 */
int charging_manager_callback(uint8_t event, uint16_t *status);

/**
 * @brief This function processes command related to this frame work
 *
 * @param  command is charger_framework_command to be processed.
 *
 * @retval None.
 */
void charger_framework_command(void *command);

#endif /*_CHARGER_FRAMEWORK_H_ */

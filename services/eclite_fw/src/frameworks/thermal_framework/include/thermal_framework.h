/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file thermal_framework.h
 *
 * @brief This file provides interface to control thermal sensor/devices.
 */

#ifndef __THERMAL_FRAMEWORK_H__
#define __THERMAL_FRAMEWORK_H__

#include <stdint.h>
#include <logging/log.h>
#include <platform.h>

/** Macro defines threshold for CPU temperature beyond which FAN should rotate.
 */
#define CPU_HIGH_TEMPERATURE 55

/** Macro defines threshold for CPU temperature below which FAN should  not
 *  rotate.
 */
#define CPU_LOW_TEMPERATURE 50

/** Macro defines threshold for CPU temperature beyond which EClite application
 *  requests shutdown.
 */
#define CPU_CRITICAL_TEMPERATURE 100

/** Macro defines delta alert for CPU temperature above which EClite application
 *  sends notification to OS.
 */
#define CPU_TEMP_DELTA 3

/** Macro defines high alert default threshold for system thermistor
 */
#define THM_HIGH_TEMPERATURE 55

/** Macro defines low alert default threshold for system thermistor
 */
#define THM_LOW_TEMPERATURE 10

/** Macro defines threshold for system temperature beyond which EClite
 *  application requests shutdown.
 */
#define THM_CRITICAL_TEMPERATURE 90

/** @brief Structure to process thermal commands.
 *
 *  Structure is used to update parameter sent by host via opregion
 */
struct thermal_command_data {
	/** Thermal command message id. This value is update to opregion*/
	uint8_t event_id;
	/** command parameter to receive parameter from HOST.*/
	int16_t data[MAX_THERMAL_SENSOR * 2];
};


/** @brief Thermal devices
 *
 *  Enum "thermal_device" used to identify which device has generated an alert.
 *
 */
enum thermal_device {
	SENSOR = 0,
	CPU = 15,
};

/** @brief Thermal status for Thermal devices
 *
 *  Enum "thermal_status" provides a way to classify thermal condition so these
 *  condition can be notified to host
 *
 *  thermal framework need to mark this condition as a part of the status.
 */
enum thermal_status {
	THERMAL_HIGH_THRESHOLD = 0,
	THERMAL_LOW_THRESHOLD,
	THERMAL_CRITICAL_THRESHOLD,
	CPU_HIGH_THRESHOLD,
	CPU_LOW_THRESHOLD,
	CPU_CRITICAL_THRESHOLD,
};

/** @brief Thermal sensor configuration detail
 *
 *  Structure contains parameter for sensor configuration. these parameters are
 *  physical GPIO/I2C detail, parameter useful for thermal frame work which are
 *  different temperature threshold and temperature reading from sensor.
 */
struct thermal_driver_data {
	/** GPIO configuration.*/
	uint8_t gpio_pin;
	/** Sensor I2C slave address.*/
	uint8_t i2c_slave;
	/** Threshold to generate an alert if temperature exceeds.*/
	uint16_t high_threshold;
	/** Threshold to generate an alert if temperature falls.*/
	uint16_t low_threshold;
	/** Threshold to generate an alert to indicate shutdown.*/
	uint16_t critical;
	/** Current temperature.*/
	int16_t temperature;
	/** Fan Rotation. */
	int16_t rotation;
	/** Fan Rotation. */
	int16_t tacho;
	/** Thermal Status  Below is bit configuration for status.
	 *  BIT 7  - 0: Thermal status: reposted status is not device specific
	 *  and can be overwritten by other sensor. purpose of this is just to
	 *  indicated sensor status.
	 *  BIT 15 - 8: Instance: Purpose of this is to indicate which sensor
	 *  has changes Thermal status. This can be used to determine which
	 *  sensor has caused an thermal event.
	 */
	uint16_t status;
};

/**
 * @brief This function is used to read temperature from thermal sensor.Fan
 * rotation from tacho.
 *
 * This function is used by thermal driver. Other thermal APIs has same
 * prototype unless defined explicitly.
 *
 * @param dev is eclite device pointer for charger.
 * @param data is pointer to read temperature.
 *
 * @retval I2C driver error codes.
 */

typedef int (*read_data_t)(void *dev, int16_t *data);

/**
 * @cond INTERNAL_HIDDEN
 *
 * This defines are used by charging manager internally to access fuel gauge.
 */

typedef int (*write_data_t)(void *dev, int16_t *data);
typedef int (*status_data_t)(void *dev, int16_t *data);
typedef int (*configure_data_t)(void *dev, int16_t *data);

/**
 * @endcond
 */

/** @brief Thermal sensor configuration detail
 *
 *  Structure contains APIs which can fetch/process thermal devices
 */
struct thermal_driver_api {
	/** read from thermal device.*/
	read_data_t read_data;
	/** write to thermal device.*/
	write_data_t write_data;
	/** write to thermal device.*/
	status_data_t status_data;
	/** Configure thermal sensor.*/
	configure_data_t configure_data;
};

/**
 * @brief This function is to process command sent by HOST
 *
 * @param command is thermal_command_data.
 *
 * @retval None.
 */
void thermal_command(void *command);

/**
 * @brief Thermal callback
 *
 * This function is called periodically to process data from the thermal
 * devices. This function read thermal data from different sensor and CPU. based
 * on the collected data it controls fan. Fan rotation feedback measure using
 * tachometer.
 *
 * @param status is output from framework. which indicates which device has
 * created an alert.
 *
 * @retval Error Codes.
 */
int thermal_callback(uint16_t *status);

#ifdef CONFIG_ECLITE_THERMAL_DEBUG

/** EClite thermal framework INFO macro.*/
#define ECLITE_THERMAL_INFO(param ...) printk("[ECLITE-THERMAL-INFO]: " param)
/** EClite thermal framework DEBUG macro.*/
#define ECLITE_THERMAL_DEBUG(param ...) printk("[ECLITE-THERMAL-DEBUG]: " \
					       param)
/** EClite thermal framework ERROR macro.*/
#define ECLITE_THERMAL_ERROR(param ...) printk("[ECLITE-THERMAL-ERROR]: " \
					       param)

#else
/** EClite thermal framework INFO macro.*/
#define ECLITE_THERMAL_INFO(param ...)
/** EClite thermal framework DEBUG macro.*/
#define ECLITE_THERMAL_DEBUG(param ...)
/** EClite thermal framework ERROR macro.*/
#define ECLITE_THERMAL_ERROR(param ...)

#endif  /* _ECLITE_THERMAL_DEBUG_ */

#endif  /* __THERMAL_FRAMEWORK_H__ */

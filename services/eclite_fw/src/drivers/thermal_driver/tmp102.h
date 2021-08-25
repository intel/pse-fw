/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TMP102_H_
#define _TMP102_H_
/**
 * @file thermal device TMP102 device interface file
 *
 * @brief This file provides interface to control thermal sensor/devices TMP102.
 */
#include <logging/log.h>
#include <eclite_device.h>
enum tmp102_temperature_format {
	TMP102_REG_FORMAT = 0,
	TMP102_READABLE_FORMAT,
};

/** @brief TMP102 register map
 *
 *  Enum "tmp102_register_map" provides register map for TMP102.
 */
enum tmp102_register_map {
	/** Read Only.*/
	TMP102_TEMPERATURE_REG = 0X0,
	/** Read / Write.*/
	TMP102_CONFIGURATION_REG,
	TMP102_T_LOW_REG,
	TMP102_T_HIGH_REG,
};

enum tmp102_conversion_rate {
	TMP102_CONVERSION_0_25_HZ = 0,
	TMP102_CONVERSION_1_HZ,
	TMP102_CONVERSION_4,
	TMP102_CONVERSION_8,
};

enum tmp102_mode_rate {
	TMP102_EM_NORAML_MODE = 0,
	TMP102_EM_EXTENDED_MODE,
};

enum tmp102_thermostat_mode_rate {
	TMP102_TM_COMPARATOR_MODE = 0,
	TMP102_TM_INTERRUPT_MODE,
};

enum tmp102_shutdown_mode {
	TMP102_SD_ACTIVE_MODE = 0,
	TMP102_SD_SHUTDOWN_MODE,
};

enum tmp102_active_pin_polarity {
	TMP102_POL_ACTIVE_LOW = 0,
	TMP102_POL_ACTIVE_HIGH,
};

enum tmp102_fault_queue {
	TMP102_FX_RETRY_1 = 0,
	TMP102_FX_RETRY_2,
	TMP102_FX_RETRY_4,
	TMP102_FX_RETRY_6,
};

struct tmp102_configuration {

	/** MSB.*/
	/** shutdown mode.*/
	uint8_t shutdown_mode : 1;
	/** thermostat mode.*/
	uint8_t thermostat_mode : 1;
	/** polarity.*/
	uint8_t pol : 1;
	/** fault queue.*/
	uint8_t fault_queue : 2;
	/** converster resulution bit(read only).*/
	uint8_t converter_resolution : 2;
	/** onse shot conversion. This is useful in shutdown mode.*/
	uint8_t one_shot : 1;

	/** LSB.*/
	/** reserved bit fields.*/
	uint8_t reserved_1 : 4;
	/** It decides temperature format.*/
	uint8_t extended_mode : 1;
	/** alert status read only.*/
	uint8_t alert_status : 1;
	/** converstion rate. default is 4Hz.*/
	uint8_t conversion_rate : 2;

};

/** Tmp102 sensor devices.*/
extern struct eclite_device tmp102_sensor1;
extern struct eclite_device tmp102_sensor2;
extern struct eclite_device tmp102_sensor3;
extern struct eclite_device tmp102_sensor4;

#endif /* _TMP102_H_ */

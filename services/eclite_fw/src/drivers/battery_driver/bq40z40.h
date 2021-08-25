/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief This file exposes interface for charger BQ40Z40.
 */

#ifndef _BQ40Z40_H_
#define _BQ40Z40_H_

#include <device.h>
#include <logging/log.h>
#include <stdint.h>

/** bq40z40 fuel gauge device.*/
extern struct eclite_device bq40z40_device_fg;

/** @brief register map for BQ40Z40.
 *
 *  Enum "bq40z40_battery_register" provides register map for BQ40Z40.
 */
enum bq40z40_battery_register {
	BQ40Z40_AT_RATE_REG = 0x4,
	BQ40Z40_VOLTAGE_REG = 0x9,
	BQ40Z40_CURRENT_REG = 0xA,
	BQ40Z40_RELATIVE_STATE_OF_CHARGE_REG = 0xD,
	BQ40Z40_ABSOLUTE_STATE_OF_CHARGE_REG = 0xE,
	BQ40Z40_REMAINING_BATTERY_REG = 0xF,
	BQ40Z40_FULL_CHARGE_CAPACITY_REG = 0x10,
	BQ40Z40_CHARGE_VOLTAGE_REG = 0x14,
	BQ40Z40_CHARGE_CURRENT_REG = 0x15,
	BQ40Z40_ALARM_WARNING_REG = 0x16,
	BQ40Z40_DESIGN_CAPACITY_REG = 0x18,
	BQ40Z40_CYCLE_COUNT_REG = 0x17,
	BQ40Z40_DESIGN_VOLTAGE_REG = 0x19,
	BQ40Z40_BTP_DISCHARGE = 0x4A,
	BQ40Z40_BTP_CHARGE = 0x4B,
	BQ40Z40_MFG_ACCESS = 0x0,
	BQ40Z40_MFG_BLOCK_ACCESS = 0x44,
	BQ40Z40_MFG_OP_STATUS = 0x0054,
	BQ40Z40_DATAFLASH_IO_CONFIG = 0x47CC,
	BQ40Z40_MFG_DATA_BUF_SIZE = 0x20,
	BQ40Z40_BTP_EN = 0x1,
	BQ40Z40_BTP_POL = 0x2,
	BQ40Z40_EM_SHUT = 0x20,
	BQ40Z40_BAT_INIT_DONE = 0x80,
	BQ40Z40_BAT_STATUS = 0x16,
};

/** @brief GPIO configuration per GPIO.
 *
 * battery status bit map.
 */
struct  bq40z40_alarm_warning_reg {
	/** Reserved.*/
	uint8_t reserved1;
	/** Reserved.*/
	uint8_t reserved2 : 4;
	/** over temperature alarm.*/
	uint8_t over_temprature_alarm : 1;
	/** Reserved.*/
	uint8_t reserved3 : 1;
	/** discharge alarm.*/
	uint8_t terminate_discharge_alarm : 1;
	/** overcharged alarm.*/
	uint8_t over_charged_alarm_alarm : 1;
};
#endif /*_BQ40Z40_H_ */

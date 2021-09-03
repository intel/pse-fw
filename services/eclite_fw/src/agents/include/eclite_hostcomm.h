/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file eclite_hostcomm.h
 *
 * @brief eclite host communication handling.
 */

#ifndef _ECLITE_HOSTCOMM_H_
#define _ECLITE_HOSTCOMM_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <logging/log.h>
#include "ucsi.h"
/**
 * @cond INTERNAL_HIDDEN
 *
 * This defines are used by eclite hostcommunicaiton driver internally.
 */

#define HECI_CLIENT_ECLITE_GUID  { 0x6a19cc4b, 0xd760, 0x4de3, { 0xb1, 0x4d, \
								 0xf2, 0x5e, \
								 0xbd, 0xf,  \
								 0xbc, 0xd9 } }

#define ECLITE_HECI_MAX_CONNECTIONS             0x01
#define ECLITE_HECI_PROTOCOL_VER                0x01

#define MAX_OPREGION_LENGTH                     0x180
#define NUM_OF_OPR_WR_BLKS                      0x8

#define ECLITE_EVENT_CHARGER_CONNECT            0x01
#define ECLITE_EVENT_CHARGER_DISCONNECT         0x02
#define ECLITE_EVENT_BATTERY                    0x03
#define ECLITE_EVENT_SYSTEMP0_ALERT             0x04
#define ECLITE_EVENT_SYSTEMP1_ALERT             0x05
#define ECLITE_EVENT_SYSTEMP2_ALERT             0x06
#define ECLITE_EVENT_BAT0TEMP_ALERT             0x07
#define ECLITE_EVENT_BAT1TEMP_ALERT             0x08
#define ECLITE_EVENT_PMICTEMP_ALERT             0x09
#define ECLITE_EVENT_SYSTEMP3_ALERT             0x0A
#define ECLITE_EVENT_UCSI_UPDATE                0x0C
#define ECLITE_EVENT_CYCLE_COUNT_CHANGE         0x0E
#define ECLITE_EVENT_CPUTEMP_UPDATE             0x0F

#define ECLITE_OS_EVENT_BTP_UPDATE              0x01
#define ECLITE_OS_EVENT_CRIT_BAT_UPDATE         0x02
#define ECLITE_OS_EVENT_LOW_BAT_TH_UPDATE       0x03
#define ECLITE_OS_EVENT_CRIT_TEMP_UPDATE        0x04
#define ECLITE_OS_EVENT_THERM_ALERT_TH_UPDATE   0x06
#define ECLITE_OS_EVENT_UCSI_UPDATE             0x0A
#define ECLITE_OS_EVENT_CHG1_UPDATE             0x0C
#define ECLITE_OS_EVENT_PWM_UPDATE              0x0F

#define ECLITE_HECI_MAX_RX_SIZE         (MAX_OPREGION_LENGTH + \
					 sizeof(struct message_header_type))
#define ECLITE_HECI_INVALID_CONN_ID             -1
/* Total Op-region size is 384 bytes */
#define MAX_OPR_LENGTH                          0x180 /* 384 */

#define ECLITE_OPR_REVISION                     0x0
#define ECLITE_HEADER_TYPE_DATA                 0x1
#define ECLITE_HEADER_TYPE_EVENT                0x2
#define ECLITE_HEADER_OPR_READ                  0x1
#define ECLITE_HEADER_OPR_WRITE                 0x2

#define ECLITE_BTP1_START_OFFSET \
	offsetof(struct eclite_opregion_t, battery_info.trip_point[0])
#define ECLITE_BTP1_END_OFFSET \
	(offsetof(struct eclite_opregion_t, battery_info.trip_point[1]) + 1)
#define ECLITE_BTP_BLKSZ \
	((ECLITE_BTP1_END_OFFSET - ECLITE_BTP1_START_OFFSET) + 1)

#define ECLITE_SYSTH_THR_START_OFFSET \
	offsetof(struct eclite_opregion_t, sys0_alert3_1)
#define ECLITE_SYSTH_THR_END_OFFSET \
	(offsetof(struct eclite_opregion_t, sys3_alert1_1) + 1)
#define ECLITE_SYSTH_BLKSZ \
	((ECLITE_SYSTH_THR_END_OFFSET - ECLITE_SYSTH_THR_START_OFFSET) + 1)

#define ECLITE_TH_CRIT_THR_START_OFFSET	\
	offsetof(struct eclite_opregion_t, sys0_crittemp_thr1)
#define ECLITE_TH_CRIT_THR_END_OFFSET \
	(offsetof(struct eclite_opregion_t, sys3_crittemp_thr1) + 1)
#define ECLITE_SYSTH_CRIT_BLKSZ	\
	((ECLITE_TH_CRIT_THR_END_OFFSET - ECLITE_TH_CRIT_THR_START_OFFSET) + 1)

#define ECLITE_CPU_CRIT_THR_START_OFFSET \
	offsetof(struct eclite_opregion_t, cpu_crittemp_thr)
#define ECLITE_CPU_CRIT_THR_END_OFFSET \
	(offsetof(struct eclite_opregion_t, cpu_crittemp_thr) + 1)
#define ECLITE_CPU_CRIT_BLKSZ		   \
	((ECLITE_CPU_CRIT_THR_END_OFFSET - \
	  ECLITE_CPU_CRIT_THR_START_OFFSET) + 1)

#define ECLITE_CHG_PAR_START_OFFSET \
	offsetof(struct eclite_opregion_t, chg1_cc)
#define ECLITE_CHG_PAR_END_OFFSET \
	(offsetof(struct eclite_opregion_t, chg1_ilim) + 1)
#define ECLITE_CHG_PAR_BLKSZ \
	((ECLITE_CHG_PAR_END_OFFSET - ECLITE_CHG_PAR_START_OFFSET) + 1)

#define ECLITE_FAN_PWM_START_OFFSET \
	offsetof(struct eclite_opregion_t, pwm_dutycyle)
#define ECLITE_FAN_PWM_END_OFFSET \
	(offsetof(struct eclite_opregion_t, pwm_dutycyle) + 1)
#define ECLITE_FAN_PWM_BLKSZ \
	((ECLITE_FAN_PWM_END_OFFSET - ECLITE_FAN_PWM_START_OFFSET) + 1)

#define ECLITE_ALERT_CTL_START_OFFSET \
	offsetof(struct eclite_opregion_t, event_notify_config)
#define ECLITE_ALERT_CTL_END_OFFSET \
	(offsetof(struct eclite_opregion_t, event_notify_config) + 1)
#define ECLITE_ALERT_CTL_BLKSZ \
	((ECLITE_ALERT_CTL_END_OFFSET - ECLITE_ALERT_CTL_START_OFFSET) + 1)

#define ECLITE_UCSI_START_OFFSET \
	offsetof(struct eclite_opregion_t, ucsi_in_data.msg_out_info.msg[0])
#define ECLITE_UCSI_END_OFFSET \
	(offsetof(struct eclite_opregion_t, ucsi_in_data.ucsi_version) + 1)
#define ECLITE_UCSI_BLKSZ \
	((ECLITE_UCSI_END_OFFSET - ECLITE_UCSI_START_OFFSET) + 1)
/**
 * @endcond
 */

/** @brief host communication message header.
 *
 * host communication header message.
 */
struct message_header_type {
	/** Header revision */
	uint32_t revision : 2;
	/** Data/Event notificaiton */
	uint32_t data_type : 2;
	/** Data Read/Write */
	uint32_t read_write : 2;
	/** Data Write Offset */
	uint32_t offset : 9;
	/** Data payload size in bytes */
	uint32_t length : 9;
	/** Event notification number */
	uint32_t event : 8;
};

/** @brief battery information.
 *
 * battery information table.
 */
struct battery {
	/** battery state */
	uint16_t state;

	/** battery present rate of charge/discharge */
	uint16_t preset_rate;

	/** battery remaining capacity */
	uint16_t remaining_capacity;

	/** battery voltage */
	uint16_t battery_voltage;

	/** battery design capacity */
	uint16_t design_capacity;

	/** battery full charge capacity */
	uint16_t full_charge_capacity;

	/** battery design voltage */
	uint16_t design_voltage;

	/** battery cycle count */
	uint16_t cycle_count;

	/** battery design capacity warning */
	uint16_t design_capacity_warning;

	/** battery design capacity for low */
	uint16_t design_capacity_low;

	/** battery trip point form OS */
	uint16_t trip_point[2];

	/** battery discharge rate */
	uint16_t discharge_rate;

	/** battery warning level */
	uint16_t warning_level;

	/** battery critical level */
	uint16_t critical_level;

	/** battery reserved */
	uint16_t reserved[4];
};

struct ucsi_info_in {
	/** UCSI msg out information 260-275*/
	struct msg_out_t msg_out_info;

	/** UCSI control information 276-283*/
	struct control_t control_info;

	/** UCSI msg_in information 284-299*/
	struct msg_in_t msg_in_info;

	/** UCSI cci information 300-303*/
	struct cci_t cci_info;

	/** UCSI version number 304-305*/
	uint16_t ucsi_version;
};

/** @brief operation region for host communication.
 *
 * operation region for exchanging data with OS/Host driver.
 */
struct eclite_opregion_t {
	/** Battery information 0-37 */
	struct battery battery_info;

	/** reserved 38-75 */
	uint16_t reserved_1[19];

	/** system thermistor0 temperature1 -76-77 */
	uint16_t systherm0_temp1;

	/** system thermistor1 temperature2 -78-79 */
	uint16_t systherm1_temp1;

	/** system thermistor2 temperature1 -80-81 */
	uint16_t systherm2_temp1;

	/** system thermistor3 temperature1 -82-83 */
	uint16_t systherm3_temp1;

	/** reserved -84-85 */
	uint16_t reserved_2;

	/** reserved -86-87 */
	uint16_t reserved_3;

	/** reserved -88-99 */
	uint16_t reserved_4[6];

	/** system thermistor0 alert 3 -100-101 */
	uint16_t sys0_alert3_1;

	/** system thermistor0 alert 1 -102-103 */
	uint16_t sys0_alert1_1;

	/** system thermistor1 alert 3 -104-105 */
	uint16_t sys1_alert3_1;

	/** system thermistor1 alert 1 -106-107 */
	uint16_t sys1_alert1_1;

	/** system thermistor2 alert 3 -108-109 */
	uint16_t sys2_alert3_1;

	/** system thermistor2 alert 1 -110-111 */
	uint16_t sys2_alert1_1;

	/** system thermistor3 alert 3 -112-113 */
	uint16_t sys3_alert3_1;

	/** system thermistor3 alert 1 -114-115 */
	uint16_t sys3_alert1_1;

	/** reserved -116-119 */
	uint16_t reserved_5[2];

	/** reserved -120-123 */
	uint16_t reserved_6[2];

	/** resreved -124-147 */
	uint16_t reserved_7[12];

	/** system thermistor 0 critical threshold 1 -148-149 */
	uint16_t sys0_crittemp_thr1;

	/** system thermistor 1 critical threshold 1 150-151 */
	uint16_t sys1_crittemp_thr1;

	/** system thermistor 2 critical threshold 1 152-153 */
	uint16_t sys2_crittemp_thr1;

	/** system thermistor 3 critical threshold 1 154-155 */
	uint16_t sys3_crittemp_thr1;

	/** reserved -156-157 */
	uint16_t reserved_8;

	/** reserved 158-159 */
	uint16_t reserved_9;

	/** reserved 160-173 */
	uint16_t reserved_10[7];

	/** critical temperature threshold 174-175 */
	uint16_t cpu_crittemp_thr;

	/** CPU temperature 176-177 */
	uint16_t cpu_temperature;

	/** charger CC 178-179 */
	uint16_t chg1_cc;

	/** charger CV 180-181 */
	uint16_t chg1_cv;

	/** charger input current limit 182-183 */
	uint16_t chg1_ilim;

	/** reserved 184-185 */
	uint16_t reserved_11;

	/** reserved 186-187 */
	uint16_t reserved_12;

	/** reserved 188-189 */
	uint16_t reserved_13;

	/** reserved 190-207 */
	uint16_t reserved_14[9];

	/** reserved 208-209 */
	uint16_t reserved_15;

	/** PSRC 210-211 */
	uint16_t psrc;

	/** reserved 212-223 */
	uint16_t reserved_16[6];

	/** reserved 224-225 */
	uint16_t reserved_17;

	/** reserved 226-227 */
	uint16_t reserved_18;

	/** reserved 228-241 */
	uint16_t reserved_19[7];

	/** reserved 242-243 */
	uint16_t reserved_20;

	/** reserved 244-257 */
	uint16_t reserved_21[7];

	/** reserved 258-259 */
	uint16_t reserved_22;

	/** UCSI related information 260-305*/
	struct ucsi_info_in ucsi_in_data;

	/** reserved 306-321 */
	uint16_t reserved_24[8];

	/** reserved 322-323 */
	uint16_t reserved_25;

	/** reserved 324-325 */
	uint16_t reserved_26;

	/** reserved 326-327 */
	uint16_t reserved_27;

	/** FAN PWM Duty Cycle 328-329 */
	uint16_t pwm_dutycyle;

	/** FAN Tacho reading in rpm 330-331 */
	uint16_t tacho_rpm;

	/** reserved 332 */
	uint8_t reserved_28;

	/** enable host event update 333 */
	uint8_t event_notify_config;

	/** reserved 334-384 */
	uint16_t reserved_29[25];
};

/** @brief Message data buffer for host communication.
 *
 * Message data buffer for host communication.
 */
struct message_buffer {

	/** message header */
	struct message_header_type message_header;

	/** data buffer (maximum length 384 bytes */
	uint8_t data[MAX_OPR_LENGTH];
};

/** @brief Write permission lookup table for opregion.
 *
 * Write permission lookup table for operegion variables.
 */
struct eclite_opreg_wr_attr_tbl {

	/** offset start pointer */
	uint16_t offset_start;

	/** offset end pointer */
	uint16_t offset_end;

	/** write block size */
	uint16_t block_sz;
};

/**
 * @brief This routine process event message received from BIOS.
 *
 * @param event event number from bios.
 *
 * @retval SUCCESS event processed.
 * @retval ERROR error in processing event.
 */
int eclite_heci_event_process(uint32_t event);

/**
 * @brief This routine create and initializes HECI host communication interface.
 *
 * @retval 0 host initialization successful.
 * @retval error in heci registration or initialization
 */
int init_host_communication(void);

/**
 * @brief This routine sends event notification to BIOS for various ec lite
 *		events.
 *
 * @param event event number for bios.
 *
 * @retval 0 event send successfully.
 * @retval error in sending event.
 */
int eclite_send_event(uint32_t event);

/**
 * @brief eclite operation region table
 */
extern struct eclite_opregion_t eclite_opregion;
extern bool cpu_thermal_enable;
extern uint32_t heci_connection_id;

#endif /*_ECLITE_HOSTCOMM_H_ */

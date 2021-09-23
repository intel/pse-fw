/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file utils.h
 *
 * \brief This file contains all declarations and macro definiitons &
 * preprocesor directives needed as common include for all files
 * in managability
 *
 */

#ifndef _COMMON_UTILS_H_
#define _COMMON_UTILS_H_

#include "oob_errors.h"
#include "pse_app_framework.h"
#include <zephyr/types.h>

#define GENERAL_BUF_SIZE            512
#define BIG_GENERAL_BUF_SIZE       1024

/* OOB Variable Definer */
#if defined(CONFIG_SOC_INTEL_PSE)
#define VAR_DEFINER \
	APP_GLOBAL_VAR(2)

#define VAR_DEFINER_BSS	\
	APP_GLOBAL_VAR_BSS(2)
#endif

/** Application wait times(millisecs) */
#define APP_MINI_LOOP_TIME         1000

/** Meaningful string to print instead of RC_STR */
#define RC_STR(rc)      ((rc) == 0 ? "OK" : "ERROR")

/** enum type used in extract_adapter and henceforth to use pass
 * around the cloud_adapter enum type
 */
enum cloud_adapter { NOADAPTER = -1, TELIT, AZURE, THINGSBOARD };

/** enum messages to identify the type of incoming messages from server.
 * Currently these are the only types for all possible incoming messages
 * from cloud adapter of any type.
 */
enum oob_messages { REBOOT, DECOMMISSION, POWERON, POWEROFF, IGNORE,
		    RELAY, PLT_STATE_CHANGE_EVENT, PLT_STATE_TIMER_EVENT,
		    TLS_SESSION_STATE_EVENT, MQTT_STATE_CHANGE_EVENT,
		    PLT_NW_MGMT_EVENT };

/** enum pmc_messages to identify different kinds of
 * power management commands
 */
enum pmc_messages { PMC_ACTION_REBOOT_HOST,
		    PMC_ACTION_REBOOT_PLATFORM,
		    PMC_ACTION_POWERON,
		    PMC_ACTION_POWEROFF };

/**
 * managability fifo queue
 *
 * @details This is first-in-first-out queue used to share messages between
 * MQTT publish_rx_tc callback thread and wait_for_events which run in
 * the main thread. The message is by itself is cloud agnostic.
 */
struct fifo_message {
	void *fifo_reserved;
	uint8_t *next_msg;
	enum oob_messages current_msg_type;
};

/**
 * enum type used to differentiate outgoing messages
 */
enum app_message_type { STATIC, DYNAMIC, EVENT, API };

/**
 * enum type to identify or track the pm state status
 */
enum pm_status { ACTIVE_POWER, LOW_POWER };

/**
 * enum type to identify or track TLS session & NW event status
 */
enum oob_conn_state { NW_MGMT_EVENT_INIT,
		      NW_MGMT_EVENT_RECEIVED,
		      NW_MGMT_EVENT_STOP,
		      TLS_SESSION_INIT,
		      TLS_SESSION_RECONNECT };


#endif /* endif _COMMON_UTILS_H */

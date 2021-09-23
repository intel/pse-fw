/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file ehl-oob.h
 *
 * \brief This contains the headers needed by ehl-oob.
 *
 */

#ifndef _EHL_OOB_H_
#define _EHL_OOB_H_

#include "common/utils.h"

#define OOB_THREAD_STACK_SIZE           4096
#define OOB_POOL_MIN_SIZE_BLOCK         8
#define OOB_POOL_MAX_SIZE_BLOCK         128
#define OOB_POOL_MAX_BLOCKS             4
#define OOB_POOL_BUF_ALIGN_VALUE        4

/* OOB API error */
#define OOB_API_MSG_ERR                 "OOB API message could not sent"

/* OOB CMD details */
#define OOB_REBOOT_CMD                  "Reboot command Triggered"
#define OOB_EVENT_MSG_ERR               "OOB event message could not sent"
#define OOB_POWER_ON_TRIGGER            "Poweron command Triggered"
#define OOB_POWER_OFF_TRIGGER           "Poweroff command Triggered"
#define OOB_DECOMMISSION_TRIGGER        "Decomission command Triggered"

/* OOB CMD Status */
#define OOB_REBOOT_SUCCESS              "Platform Reboot Initiated"
#define OOB_REBOOT_FAILED               "Platform Reboot Failed"
#define OOB_PMC_POWERON_SUCCESS         "Platform Power On Initiated"
#define OOB_PMC_POWERON_FAILED          "Platform Power On Failed"
#define OOB_PMC_POWEROFF_SUCCESS        "Platform Power Off Initiated"
#define OOB_PMC_POWEROFF_FAILED         "Platform Power Off Failed"
#define OOB_DECOMMISSION_SUCCESS        "Platform Decommission Successful"
#define OOB_DECOMMISSION_FAILED         "Platform Decommission Failed"
#define OOB_DECOMMISSION_EN_SUCCESS     "Platform Decommission Initiated"
#define OOB_DECOMMISSION_EN_FAILED      "Platform Decommission Initiate Failed"
#define OOB_REBOOT_IGNORE               "Platform in Low Power state - " \
	"Ignoring REBOOT command. Please Retry with POWER-ON."
#define OOB_DECOMMISSION_IGNORE         "Platform in Low Power state - " \
	"Decommission not supported in Low Power state."		 \
	"Ignoring Decommission. Please Retry with POWER-ON."		 \

/* OOB PM Sx event notification */
#define OOB_PM_SX_ENTRY                 "Platform in Low Power state"
#define OOB_PM_SX_EXIT                  "Platform in Active Power state"
#define OOB_PM_LOW_PWR_STATE            "Platform already in Low power state"
#define OOB_PM_ACTIVE_PWR_STATE         "Platform already in Active power state"
#define OOB_PM_SX_TRANSITION_FAILED     "Platform power state transition " \
	"taking longer than expected. Please retry"

/*
 * oob function details
 */
int ehl_oob_bootstrap(void);
int post_message(char *payload, enum app_message_type type);
int send_telemetry(char *key, char *value, enum app_message_type type);
void oob_set_pm_control(bool set_oob);
void oob_tls_session_expiry(struct k_timer *timer);
void oob_sx_trans_expiry(struct k_timer *timer);
void oob_power_trans_expiry(struct k_timer *timer);
int pmc_action(enum oob_messages pmc_action);

#endif

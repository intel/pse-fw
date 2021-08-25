/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief This file exposes interface for charger BQ24610.
 */

#ifndef _BQ24610_H_
#define _BQ24610_H_
#include <logging/log.h>
#include "platform.h"

/** GPIO to detect AC presence.*/
#define CHARGER_GPIO_AC_PRESENT         CHARGER_GPIO
/** GPIO to control charging.*/
#define CHARGER_GPIO_CE                 CHARGER_CONTROL

/** Enable Charging.*/
#define ENABLE_CHARGING                 1
/** Disable charging.*/
#define DISABLE_CHARGING                0

/** bq24610 charger device.*/
extern struct eclite_device bq24610_device_charger;

#endif /*_BQ24610_H_ */

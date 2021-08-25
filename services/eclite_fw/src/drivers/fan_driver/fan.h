/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Fan device interface file.
 *
 * @brief This file provides interface to fan device.
 */

#ifndef _FAN_H_
#define _FAN_H_
#include <logging/log.h>
#include <eclite_device.h>

#define ECLITE_PWM_PIN 0
/** 25 kHz */
#define ECLITE_PWM_FREQ 4000
/** Fan device.*/
extern struct eclite_device fan_device;
#endif /* _FAN_H_ */

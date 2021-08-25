/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief This file exposes APIs to access opregion
 */

#ifndef _ECLITE_OPREGION_H_
#define _ECLITE_OPREGION_H_

#include <device.h>
#include <logging/log.h>
#include <stdint.h>

/**
 * @brief  This function is used to update communicate data with host/OS.
 *
 * @param  device :is eclite device id for which opregion need to be updated.
 *
 * @retval None.
 */
void update_opregion(uint8_t device);

/**
 * @brief  This function is used to initialize opregion data.
 *
 * @param  device :is eclite device id for which opregion need to be initialized
 *
 * @retval None.
 */
void initialize_opregion(uint8_t device);

#endif /* _ECLITE_OPREGION_H_ */

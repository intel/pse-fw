/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <irq.h>
#include <pm/pm.h>
#include <sedi.h>
#include <logging/log_core.h>
#ifndef _PMA_SERVICE_H_
#define _PMA_SERVICE_H_

/**
 * @brief PM Services APIs
 * @defgroup pm_services_interface PM Service APIs
 * @{
 */

#define PM_DBG_LOG(...)
#define PM_INFO(...)
#define NVIC_INTR_PNED_STATUS_REG (0xE000E200)

/**
 * Initiate ARM Core Clock gate.
 *
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
static inline int power_arm_cg(void)
{
	PM_DBG_LOG("%s\n ", __func__);

	return sedi_pm_set_power_state(PSE_D0i0);

}

/**
 * Get Atom device status.
 *
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
static inline int get_atom_dev_status(void)
{
	PM_DBG_LOG("%s\n ", __func__);

	return sedi_pm_check_host_device_status();
}

/**
 * Request PSE to enter sleep where PSE put Trunk level clock gate state.
 *
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
static inline int power_soc_shallow_sleep(void)
{
	sedi_pm_set_power_state(PSE_D0i1);
	PM_DBG_LOG("reason %x\n", *((uint32_t *)NVIC_INTR_PNED_STATUS_REG));

	return 0;
}

/**
 * Verify any pending interrupt in NVIC.
 *
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
static inline int verify_pend_interrupt(void)
{
	PM_DBG_LOG("%s\n ", __func__);
	return sedi_pm_check_pending_interrupt();
}

/**
 * Request PSE to enter sleep where PSE put Trunk level clock gate
 * state with SRAM in retention state.
 *
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
static inline int power_soc_sleep(void)
{
	return sedi_pm_set_power_state(PSE_D0i2);
}

/**
 * Request PSE to enter deep sleep where PSE put into power gate state.
 *
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
static inline int power_soc_deep_sleep(void)
{
	return sedi_pm_set_power_state(PSE_D0i3);
}

/**
 * @}
 */

#endif

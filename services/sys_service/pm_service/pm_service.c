/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/sys_io.h>
#include <sys/__assert.h>
#include <pm/pm.h>
#include <device.h>
#include <soc.h>
#include "sedi.h"
#include "pm_service.h"

#define PMU_WAKE_ENABLE (0x40500010)

#define LOG_LEVEL CONFIG_PM_SERV_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(pm_serv);

extern void sedi_pm_wake_isr(void);
extern void sedi_pm_pciedev_isr(void);
extern void sedi_pm_reset_prep_isr(void);
extern void sedi_pm_pmu2nvic_isr(void);
extern void pm_hpet_timer_1_isr(void);
extern void sedi_cru_clk_ack_isr(void);
void pm_hw_event_handler(void *dummy1, void *dummy2, void *dummy3);

static const struct pm_state_info pm_min_residency[] =
	PM_STATE_INFO_DT_ITEMS_LIST(DT_NODELABEL(cpu0));

void pm_power_state_set(struct pm_state_info info)
{
	PM_DBG_LOG("%s\n ", __func__);

	if (verify_pend_interrupt() != 0) {
		return;
	}

	if (get_atom_dev_status() != 0 || pm_device_is_any_busy() != 0) {
		/*if any of atom owned device or PSE peripheral is busy, then
		 * we can enter only ARM CG.
		 */
		power_arm_cg();
		return;
	}

	if (ARRAY_SIZE(pm_min_residency) < 1) {
		return;
	}

	if (info.state == PM_STATE_SUSPEND_TO_IDLE) {
		/* D0i0 or D0i1*/
		if (info.min_residency_us ==
		    pm_min_residency[0].min_residency_us) {
			power_arm_cg();
		} else if ((ARRAY_SIZE(pm_min_residency) > 1) &&
			   (info.min_residency_us ==
			    pm_min_residency[1].min_residency_us)) {
			power_soc_shallow_sleep();
		}
	} else if (info.state == PM_STATE_SUSPEND_TO_RAM) {
		/* D0i2 or D0i3*/
		if ((ARRAY_SIZE(pm_min_residency) > 2) &&
		    (info.min_residency_us ==
		     pm_min_residency[2].min_residency_us)) {
			power_soc_sleep();
		} else if ((ARRAY_SIZE(pm_min_residency) > 3) &&
			   (info.min_residency_us ==
			    pm_min_residency[3].min_residency_us)) {
			power_soc_deep_sleep();
		}
	} else {
		LOG_ERR("Power state not supported\n");
	}
}

void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	PM_DBG_LOG("%s\n ", __func__);

	irq_unlock(0);
	/* clear PRIMASK */
	__enable_irq();
}


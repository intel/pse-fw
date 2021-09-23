/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file pmc_client.c
 *
 * \brief This file is the OOB PMC client implementation
 * which interfaces with apis of the PMC service to trigger
 * system POWER UP, POWER DOWN, REBOOT etc
 *
 */

#include <common/utils.h>
#include <logging/log.h>
#if defined(CONFIG_SOC_INTEL_PSE)
#include <pmc_service.h>

/* PMC short msg codes
 * These are sent over IPC
 */
#define PMC_CMD 0x14
#define PMC_GLOBAL_POWEROFF_CODE 1
#define PMC_GLOBAL_REBOOT_CODE 2
#define PMC_HOST_REBOOT_CODE 4
#define PMC_HOST_REBOOT_WITHOUT_POWERCYCLE 8
#define PMC_HOST_POWEROFF_CODE 16
#define PMC_HOST_WAKE_CODE  32

#endif /* CONFIG_SOC_INTEL_PSE */

LOG_MODULE_REGISTER(OOB_PMC_CLIENT, CONFIG_OOB_LOGGING);

/**
 * This function is a wrapper around PMC
 * apis and is used EHL OOB to invoke
 * Power commands on the SOC
 *
 * @param [in] enum pmc_messages pmc_action
 * @retval 0 on success and any other value on failure
 *
 * \see common/utils.h
 **/
int pmc_action(enum pmc_messages pmc_action)
{
#if defined(CONFIG_SOC_INTEL_PSE)
	int status = OOB_ERR_PMC_FAILED;
#else
	int status = 0;
#endif

#if defined(CONFIG_SOC_INTEL_PSE)
	struct pmc_msg_t usr_msg = { 0 };

	usr_msg.client_id = PMC_CLIENT_OOB;

	switch (pmc_action) {
	case PMC_ACTION_POWERON:
		usr_msg.format = FORMAT_SHORT;
		usr_msg.u.short_msg.cmd_id = PMC_CMD;
		usr_msg.u.short_msg.payload = PMC_HOST_WAKE_CODE;
		break;

	case PMC_ACTION_POWEROFF:
		usr_msg.format = FORMAT_SHORT;
		usr_msg.u.short_msg.cmd_id = PMC_CMD;
		usr_msg.u.short_msg.payload = PMC_HOST_POWEROFF_CODE;
		break;

	case PMC_ACTION_REBOOT_PLATFORM:
		usr_msg.format = FORMAT_SHORT;
		usr_msg.u.short_msg.cmd_id = PMC_CMD;
		usr_msg.u.short_msg.payload = PMC_GLOBAL_REBOOT_CODE;
		break;

	case PMC_ACTION_REBOOT_HOST:
		usr_msg.format = FORMAT_SHORT;
		usr_msg.u.short_msg.cmd_id = PMC_CMD;
		usr_msg.u.short_msg.payload = PMC_HOST_REBOOT_CODE;
		break;
	}

	status = pmc_sync_send_msg(&usr_msg);
	if (status != 0) {
		LOG_ERR("Error in request processed by pmc service\n");
		if (pmc_action != PMC_ACTION_POWEROFF) {
			usr_msg.format = FORMAT_WIRE_GLOBAL_RESET;
			status = pmc_sync_send_msg(&usr_msg);
			if (status != 0) {
				LOG_ERR("Failed to trigger wired global reset");
			}
		}
	}
#endif
	return status;
}

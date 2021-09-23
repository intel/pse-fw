/**
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "driver/sedi_driver_sideband.h"
#include <logging/log.h>

LOG_MODULE_REGISTER(OOB_SEC_UNITTEST, CONFIG_OOB_LOGGING);

/*
 *	FWD [PSE][SB][IPC][HFPGA]PSE can not communicate with PMC by
 *	PSE->PMC SB in HFPGA due to SBR table setting in PSE IP-FPGA
 *	URL: https://hsdes.intel.com/resource/1507024351
 */

enum DRNG_REGS {
	EGETDATA24 = 0x0054
};

void test_sideband_drng_reg(u32_t addr, unsigned char *reg_name)
{

	u64_t read_val;
	int ret;

	LOG_INF(
		"Begin to test sideband DRNG %s\n",
		reg_name);

	ret = sedi_sideband_send(0, SB_DRNG, SEDI_SIDEBAND_ACTION_READ,
				 addr, 0);

	if (ret != 0) {
		LOG_INF(
			"Error in send! return value is 0x%04x\n",
			ret);
	}

	LOG_INF("Read command finished!\n");

	ret = sedi_sideband_wait_ack(0, SB_DRNG, SEDI_SIDEBAND_ACTION_READ,
				     &read_val);

	if (ret != 0) {
		LOG_INF(
			"Error in read! return value is 0x%04x",
			ret);
	}

	LOG_INF("Read %s value is 0x%04x\n",
		reg_name,
		(u32_t)read_val);

}

void test_sideband_drng(void)
{
	/* Enable the following if test_sideband_pmc() is not called
	 * isesi_sideband_init(0);
	 * isesi_sideband_set_power(0, ISESI_POWER_FULL);
	 */
	LOG_INF("Begin to test sideband DRNG\n");
	LOG_INF("===========================\n");

	test_sideband_drng_reg(EGETDATA24, "EGETDATA24");
	test_sideband_drng_reg(EGETDATA24, "EGETDATA24 x2");

}

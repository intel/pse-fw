/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @file
 * @brief Sample Application for ECMA-393 Network Proxy Service.
 * This example demonstates how to setup a Network Proxy Agent on DesginWare(R)
 * Cores Ethernet Quality-of-Service (DWC EQOS) GbE Port 0 by enabling
 * appropriate configuration macros in prj.conf file. Besides, this sample app
 * shows how to add shared memory support by relocating L2SRAM in
 * pse_sram.overlay. Once Host enters low power mode (S0ix), Network Proxy Agent
 * will take the ownership of DWC EQOS GbE Port 0 and reacts to incoming
 * packets according to the handling decisions configured by Host.
 * @{
 */

/**
 * @brief How to build sample application.
 * Please refer to section 6.0 in "Intel(R) Programmable Seriveces Engine SDK
 * Get Started Guide" for detailed instructions on how to build the sample
 * application.
 */

/**
 * @brief Hardware setup.
 * Please refer section 4.3.2.4 in "Intel(R) Programmable Seriveces Engine SDK
 * User Guide" for detailed instructions on how to setup the hardware for
 * Network Proxy service.
 */

/* Local Includes */
#include <sys/printk.h>
#include <sedi.h>
#include <zephyr.h>

/* @brief main function
 * Switch ARM core clock frequency and High Bandwidth (HBW) clock frequency to
 * 100 MHz at the beginning of Network Proxy service. This step is to make sure
 * Network Proxy service meets low-power energy requirement.
 *
 * Note:
 * When Network Proxy service is integrated/combined with other PSE services,
 * e.g. EClite, user should develop centralized power management API to
 * coordinate ARM core clock frequency and HBW clock frequency switching.
 */
void main(void)
{
	printk("PSE ECMA-393 Network Proxy Sample Application\n");

	/* Switch ARM core clock frequency and HBW clock frequency to 100 MHz */
	sedi_pm_switch_core_clock(CLOCK_FREQ_100M);
}

/**
 * @}
 */

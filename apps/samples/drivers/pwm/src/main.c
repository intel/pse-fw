/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample Application for PWM
 * This example demonstrates the following
 * Derive the clock rate for the PWM device
 * Generate the PWM output by setting the period and pulse width
 * in terms of clock cycles
 * Generate the PWM output by setting the period and pulse width
 * in microseconds
 * Use the PWM in counter mode
 * @{
 */

/*
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more details
 * how to build the sample codes.
 */

/*
 * @brief Hardware setup.
 * Please refer section 3.5.1 in “IntelPSE_SDK_User_Guide” for more
 * details for PWM hardware setup.
 * Note: Hardware connectivity differs when using Intel Add-In-Card(AIC).
 * Please refer to the relevant sections for PWM connectivity and channel
 * selection in AIC user manual when using Intel Add-In-Card.
 */

/* Local Includes */
#include <drivers/counter.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <drivers/pwm.h>
#include <stdio.h>
#include <sedi.h>
#include <toolchain/gcc.h>
#include <zephyr.h>

/** PWM controller 0 */
#define PWM_1 0
/** PWM controller 1 */
#define PWM_2 1
/** PWM channel 1. Each PWM controller has 8 PWM channels */
#define PWM_CHANNEL_1 0
/** PWM channel 3 */
#define PWM_CHANNEL_3 2
/** PWM channel 14 maps to PWM channel 6 of PMW controller 1 */
#define PWM_CHANNEL_14 6
/** Device name for PWM controller 0 */
#define PWM_0_NAME "PWM_0"
/** Device name for PWM controller 1 */
#define PWM_1_NAME "PWM_1"
/** time period of 20  micro seconds */
#define PERIOD_20_USEC 20
/** time period of 15 micro seconds */
#define PULSE_15_USEC 15
/** time period of 10 micro seconds */
#define PULSE_10_USEC 10
/** time period of 5 micro seconds */
#define PULSE_5_USEC 5
/** time period of 20 HW clock cycles */
#define PERIOD_HW_CYCLES 20
/** time period of 5 HW clock cycles */
#define PULSE_HW_CYCLES 5

/* Each PWM channel can be configured as a counter independently
 * So we can configure 16 counters in total(8 from PWM instance 0
 * and 8 from PWM instance 1)
 */

/* defines counter 1 to use channel 11 */
#define COUNTER_1 11
/* defines counter 2 to use channel 4 */
#define COUNTER_2 4
/** device name for counter 1 */
#define COUNTER_1_NAME "COUNTER_11"
/** device name for counter 4 */
#define COUNTER_2_NAME "COUNTER_4"
/** timer value for counter 1 */
#define COUNTER_1_VAL 0x3000000
/** timer value for counter 2 */
#define COUNTER_2_VAL 0x10000000
/** delay of 10 secs */
#define DELAY_10_SEC K_MSEC(10000)
/** delay of 10 milli seconds */
#define DELAY_MS K_MSEC(10)

/* @brief callback.
 * Call back function invoked following
 * the counter expiry
 */
void call_back(const struct device *dev, void *data)
{
	ARG_UNUSED(dev);

	printk("[%s] Timer %d expired\n", __func__, (unsigned int)data);
}

/* @brief main function
 * PWM Sample Application for PSE
 * The sample app tests the PWM IP ( in PWM and counter modes )
 * The user is required to connect a probing device like oscilloscope on
 * pin1 on PWM_0 and pin3 on PWM_0 to check the PWM output.
 * Also can connect the AIC card to drive the motor via PWM14.
 */
void main(void)
{
	const struct device *pwm_1, *pwm_2;
	const struct device *dev_1, *dev_2;
	uint64_t cl;
	int ret;
	uint8_t pwm_flag = 0;
	struct counter_top_cfg cfg;

	/* Get the device handle for PWM instance 1 */
	pwm_1 = device_get_binding(PWM_0_NAME);
	if (!pwm_1) {
		printk("%s - Bind failed\n", PWM_0_NAME);
		return;
	}
	printk("%s - Bind Success\n", PWM_0_NAME);

	/* Get the device handle for PWM instance 2 */
	pwm_2 = device_get_binding(PWM_1_NAME);
	if (!pwm_2) {
		printk("%s - Bind failed\n", PWM_1_NAME);
		return;
	}
	printk("%s - Bind Success\n", PWM_1_NAME);

	/* Get the device handle for counter 1 */
	dev_1 = device_get_binding(COUNTER_1_NAME);
	if (!dev_1) {
		printk("%s - Bind failed\n", COUNTER_1_NAME);
		return;
	}
	printk("%s - Bind Success\n", COUNTER_1_NAME);

	/* Get the device handle for counter 2 */
	dev_2 = device_get_binding(COUNTER_2_NAME);
	if (!dev_2) {
		printk("%s - Bind failed\n", COUNTER_2_NAME);
		return;
	}
	printk("%s - Bind Success\n", COUNTER_2_NAME);

	/* Gets the clock rate(cycles per second) for the PWM device */
	ret = pwm_get_cycles_per_sec(pwm_1, PWM_CHANNEL_1, &cl);
	if (ret != 0) {
		printk(" Get clock rate failed\n");
		return;
	}
	printk("Running rate: %lld\n", cl);

	/* Set the period and pulse width of the PWM output in terms
	 * of clock cycles. Expected output -Pulse with 25% duty cycle
	 */
	ret = pwm_pin_set_cycles(pwm_1, PWM_CHANNEL_1, PERIOD_HW_CYCLES,
				 PULSE_HW_CYCLES, pwm_flag);
	if (ret != 0) {
		printk("Pin set cycles failed\n");
		return;
	}

	printk("Configured pin %d (Period: %d, pulse: %d)\n", PWM_CHANNEL_1,
	       PERIOD_HW_CYCLES, PULSE_HW_CYCLES);

	k_sleep(DELAY_10_SEC);

	/* Stop the PWM output */
	pwm_pin_set_cycles(pwm_1, PWM_CHANNEL_1, 0xff, 0, 0);
	printk("Stop pulses on pin %d, No o/p produced\n", PWM_CHANNEL_1);

	k_sleep(DELAY_10_SEC);

	printk("Start pulses on pin %d, o/p restored\n", PWM_CHANNEL_1);
	/* Start the PWM output */
	pwm_pin_set_cycles(pwm_1, PWM_CHANNEL_1, 0xff, 0, 0);

	/* Set the period and pulse width of the PWM output in
	 * micro seconds. Expected output-Pulse with 50% duty cycle
	 */
	ret = pwm_pin_set_usec(pwm_1, PWM_CHANNEL_3,
			       PERIOD_20_USEC, PULSE_10_USEC, pwm_flag);
	if (ret != 0) {
		printk("Pin set usec failed\n");
		return;
	}

	printk("Configured pin %d (Period: %d, pulse: %d)\n", PWM_CHANNEL_3,
	       PERIOD_20_USEC, PULSE_10_USEC);

	/* Set the period and pulse width of the PWM output in microseconds
	 * Expected output -Pulse with 75% duty cycle.
	 * Motor rotates clockwise
	 */
	ret = pwm_pin_set_usec(pwm_2, PWM_CHANNEL_14,
			       PERIOD_20_USEC, PULSE_15_USEC, pwm_flag);
	if (ret != 0) {
		printk("Pin set cycles failed\n");
		return;
	}

	printk("Configured motor to rotate clockwise"
	       "(Period: %d, pulse: %d)\n",
	       PERIOD_20_USEC, PULSE_15_USEC);

	k_sleep(DELAY_10_SEC);

	/* Set the period and pulse width of the PWM output in microseconds.
	 * Expected output -Pulse with 50% duty cycle
	 * At 50% duty cycle motor stops rotating.
	 */
	ret = pwm_pin_set_usec(pwm_2, PWM_CHANNEL_14, PERIOD_20_USEC,
			       PULSE_10_USEC, pwm_flag);

	if (ret != 0) {
		printk("Pin set cycles failed\n");
		return;
	}

	printk("Configured to stop the motor"
	       "(Period: %d, pulse: %d)\n",
	       PERIOD_20_USEC, PULSE_10_USEC);

	k_sleep(DELAY_10_SEC);

	/* Set the period and pulse width of the PWM output in microseconds.
	 * Expected output -Pulse with 25% duty cycle.
	 * Motor rotates anti-clockwise.
	 */
	ret = pwm_pin_set_usec(pwm_2, PWM_CHANNEL_14,
			       PERIOD_20_USEC, PULSE_5_USEC, pwm_flag);
	if (ret != 0) {
		printk("Pin set cycles failed\n");
		return;
	}

	printk("Configured motor to rotate anti clockwise"
	       " (Period: %d, pulse: %d)\n",
	       PERIOD_20_USEC, PULSE_5_USEC);

	k_sleep(DELAY_10_SEC);

	/* Configure the counter in periodic mode with specific counter value */
	cfg.callback = call_back;
	cfg.user_data = (void *)COUNTER_1;
	cfg.ticks = COUNTER_1_VAL;
	cfg.flags = 1;
	ret = counter_set_top_value(dev_1, &cfg);
	if (ret != 0) {
		printk("Top value set for counter %s failed\n",
		       COUNTER_1_NAME);
		return;
	}

	/* Configure the counter in one-shot mode */
	cfg.user_data = (void *)COUNTER_2;
	cfg.ticks = COUNTER_2_VAL;
	cfg.flags = 0;
	ret = counter_set_top_value(dev_2, &cfg);
	if (ret != 0) {
		printk("Oneshot top value set for counter %s failed\n",
		       COUNTER_2_NAME);
		return;
	}

	while (1) {
		k_sleep(DELAY_MS);
	}
}

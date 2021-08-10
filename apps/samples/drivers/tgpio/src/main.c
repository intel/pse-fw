/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @file
 * @brief Sample Application for TGPIO (Time-Aware GPIO).
 * This sample application tests rate and offset adjustment for timers
 * and different modes of input and ouput for TGPIO pins.
 * @{
 */

/**
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more details on how to
 * build the sample codes.
 */

/**
 * @brief Hardware setup.
 * Please refer section 3.9.1 in “IntelPSE_SDK_User_Guide” for more details
 * on hardware setup.
 *
 * Note: Hardware connectivity differs when using Intel Add-In-Card(AIC).
 * Please refer to the relevant sections for TGPIO connectivity and channel
 * selection in AIC user manual when using Intel Add-In-Card.
 */

/* Local Includes */
#include <drivers/gpio-timed.h>
#include <sys/printk.h>
#include <random/rand32.h>
#include <sedi.h>
#include <sys/util.h>
#include <zephyr.h>

#define TGPIO_DEVICE "TGPIO_0"
#define TGPIO_TIMER TGPIO_TMT_0
#define TGPIO_TIMER_1 TGPIO_TMT_1
#define TGPIO_EDGE TGPIO_RISING_EDGE
#define REPEAT_FOREVER 0
#define REPEAT_SEC 0
#define REPEAT_NSEC 100000000U
#define INIT_TIME_SEC 100U
#define INIT_TIME_NSEC 0
#define NSECS_PER_SEC 1000000000U
#define ADJUST_NSEC 99999896U
#define PWS 6
#define EVENTS_CEILING 5
#define PPB_RATE 100000U
#define GPIO_MAX_PINS 30

#define LOOPBACK_IN_PIN 7
#define LOOPBACK_OUT_PIN 23
#define LOOPBACK_IN_PIN_1 24
#define LOOPBACK_OUT_PIN_1 26

/*
 * @brief TGPIO error handler
 *
 * This macro is invoked when functions return error
 * values because of invalid parameters or operations.
 *
 */
#define handle_error(ret)						       \
	do {								       \
		if (ret < 0) {						       \
			printk("[Error %d:] Error executing function\n", ret); \
			while (1)					       \
				;					       \
		}							       \
	} while (0)

static int adjust_frequency;

/*
 * @brief TGPIO callback handler
 *
 * This function is the the application callback
 * handler function for a TGPIO pin.
 *
 */
void callback_handler(const struct device *port, uint32_t pin,
		      struct tgpio_time ts, uint64_t ev_cnt)
{
	printk("[cb-%s][pin -%2d][ts -%2d:%9d][ec -%u]\n", port->name,
	       pin, ts.sec, ts.nsec, (uint32_t)ev_cnt);
}

/*
 * @brief Test pin availability
 *
 * This function checks if a GPIO pin
 * is available as a Timed GPIO pin.
 *
 */
int test_pin_available(const struct device *tgpio_dev)
{
	int ret1, ret2, ret3, pin;
	struct tgpio_time start, interval;

	start.sec = INIT_TIME_SEC + 1;
	start.nsec = INIT_TIME_NSEC;

	for (pin = 0; pin < GPIO_MAX_PINS; pin++) {
		/* Timestamp an event on the pin. */
		ret1 = tgpio_pin_timestamp_event(tgpio_dev, pin, TGPIO_TIMER,
					EVENTS_CEILING, TGPIO_EDGE, NULL);
		/* Count number of events in a span of time. */
		ret2 = tgpio_pin_count_events(tgpio_dev, pin, TGPIO_TIMER,
					start, interval, TGPIO_EDGE, NULL);
		/* Generates pulses on the pin. */
		ret3 = tgpio_pin_periodic_out(tgpio_dev, pin, TGPIO_TIMER,
					start, interval, REPEAT_FOREVER, PWS);
		printk("[pin - %2d] ", pin);
		printk(" %s\n", (!ret1 && !ret2 && !ret3) ? "<==Available"
		       : "<==Not available");
	}

	return 0;
}

/*
 * @brief Callback handler for adjusting frequency
 *
 * This function is the the callback handler function
 * which computes difference in rate of TMT_0(Tunable
 * Monotonous Timer 0) and TMT_1(Tunable Monotonous Timer 1).
 *
 */
void cb_adjust_frequency(const struct device *port, uint32_t pin,
			 struct tgpio_time ts, uint64_t ev_cnt)
{
	static int old, new;
	static int new_sec, new_nsec;
	static int old_sec, old_nsec;
	static int diff_sec, diff_nsec;

	if (old) {
		/* capture second value */
		new_sec = ts.sec;
		new_nsec = ts.nsec;
		new = 1;
	} else {
		/* capture first value */
		old_sec = ts.sec;
		old_nsec = ts.nsec;
		old = 1;
	}
	/*
	 * Once two subsequent timestamps are captured, the difference from
	 * reference clock's running rate is computed.
	 *
	 */
	if (old && new) {
		if (old_sec < new_sec) {
			diff_sec = new_sec - old_sec;
		}

		if (old_nsec < new_nsec) {
			diff_nsec = new_nsec - old_nsec;
		} else {
			diff_nsec = NSECS_PER_SEC - old_nsec + new_nsec;
			diff_sec--;
		}
		diff_nsec = diff_nsec - REPEAT_NSEC;
		printk("[cb-%s] adj_freq: [pin -%2d][ts -%2d:%9d][ec - %2u],"
		       " running %s with %d nsecs\n",
		       port->name, pin, ts.sec, ts.nsec, (uint32_t)ev_cnt,
		       (diff_nsec < 0) ? "slow" : "fast",
		       (diff_nsec < 0) ? -diff_nsec : diff_nsec);
		old_sec = new_sec;
		old_nsec = new_nsec;
		new = 0;
		if (diff_nsec != 0) {
			adjust_frequency = 1;
		}
	}
}

/*
 * @brief Test for adjusting frequency
 *
 * In this function TMT_0's rate is adjusted to match TMT_1's rate. Here TMT_1
 * has been used as the reference clock. Reference clock can be an external
 * source too. TMT_1 is run at specific rate and produces pulses.
 * LOOPBACK_OUT_PIN is configured to generate pulses based on TMT_1
 * and LOOPBACK_IN_PIN is configured to receive those pulses in loopback and
 * capture timestamp of TMT_0. By comparing interval pulses for
 * LOOPBACK_OUT_PIN & difference between subsequent timestamps on
 * LOOPBACK_IN_PIN, the difference in rate is calculated. The difference
 * is adjusted by calling tgpio_port_adjust_frequency() with adequete ppb value
 * to match TMT_0's rate with TMT_1's rate. The user is required to physically
 * loopback the pins LOOPBACK_OUT_PIN and LOOPBACK_IN_PIN.
 *
 */
int test_adjust_frequency(const struct device *tgpio_dev,
			  enum tgpio_timer timer)
{
	int ret;
	struct tgpio_time start, interval;

	start.sec = INIT_TIME_SEC + 1;
	start.nsec = INIT_TIME_NSEC;
	interval.sec = REPEAT_SEC;
	interval.nsec = REPEAT_NSEC;

	/* Generates pulses on the LOOPBACK_OUT_PIN. */
	ret = tgpio_pin_periodic_out(tgpio_dev, LOOPBACK_OUT_PIN, TGPIO_TIMER_1,
				     start, interval, REPEAT_FOREVER, PWS);
	handle_error(ret);
	printk("[%s] Pin: %d, Timer: %d, Scheduled at:%08d:%08d\n",
	       tgpio_dev->name, LOOPBACK_OUT_PIN, TGPIO_TIMER_1,
	       start.sec, start.nsec);

	/* Timestamp an event on LOOPBACK_IN_PIN. */
	ret = tgpio_pin_timestamp_event(tgpio_dev, LOOPBACK_IN_PIN, TGPIO_TIMER,
					1, 0, cb_adjust_frequency);
	handle_error(ret);

	ret = tgpio_port_set_time(tgpio_dev, TGPIO_TIMER_1, INIT_TIME_SEC,
				  INIT_TIME_NSEC);
	handle_error(ret);
	ret = tgpio_port_set_time(tgpio_dev, TGPIO_TIMER, INIT_TIME_SEC,
				  INIT_TIME_NSEC);
	handle_error(ret);

	/* Run TMT_1 at PPB_RATE. This is the reference clock. */
	ret = tgpio_port_adjust_frequency(tgpio_dev, TGPIO_TIMER_1, PPB_RATE);
	handle_error(ret);

	while (!adjust_frequency)
		k_sleep(K_MSEC(1));

	printk("[%s] Adjusting TMT_0 frequency\n", tgpio_dev->name);
	ret = tgpio_port_adjust_frequency(tgpio_dev, TGPIO_TIMER, PPB_RATE);
	handle_error(ret);
	adjust_frequency = 0;

	k_sleep(K_MSEC(1000));
	return 0;
}

/*
 * @brief main function
 *
 * Test rate and offset adjustment for timers
 * and test different modes of input and ouput for TGPIO pins.
 *
 */
void main(void)
{
	const struct device *dev;
	struct tgpio_time start, interval, local, ref;
	int ret;

	/* Get the device handle for TGPIO device. */
	dev = device_get_binding(TGPIO_DEVICE);
	if (!dev) {
		printk("TGPIO: Device not found\n");
		return;
	}
	printk("Bind success [%s]\n", dev->name);

	/* Check which pins are available for TGPIO */
	printk("\nTest for pin availability\n");
	printk("-------------------------\n");
	test_pin_available(dev);

	/* Adjust offset for TMT_0 */
	printk("\nTest for time adjustment\n");
	printk("-------------------------\n");
	ret = tgpio_port_set_time(dev, TGPIO_TIMER, INIT_TIME_SEC,
				  INIT_TIME_NSEC);
	handle_error(ret);
	ret = tgpio_port_get_time(dev, TGPIO_TIMER, &local.sec, &local.nsec);
	handle_error(ret);
	printk("[%s] Timer: %d, Time Now: %d:%d, Adding %d nsecs\n",
	       dev->name, TGPIO_TIMER, local.sec, local.nsec,
	       ADJUST_NSEC);
	ret = tgpio_port_adjust_time(dev, TGPIO_TIMER, ADJUST_NSEC);
	handle_error(ret);
	ret = tgpio_port_get_time(dev, TGPIO_TIMER, &local.sec, &local.nsec);
	handle_error(ret);
	printk("[%s] Timer: %d, Adjust Done, Added %d nsecs, Time Now: %d:%d\n",
	       dev->name, TGPIO_TIMER, ADJUST_NSEC, local.sec,
	       local.nsec);

	/* Adjust rate of TMT_0 to match TMT_1's rate */
	printk("\nTest for rate adjustment\n");
	printk("-------------------------\n");
	test_adjust_frequency(dev, TGPIO_TIMER);

	/* Capture timestamps from a local clock and a reference clock */
	printk("\nTest for cross timestamp\n");
	printk("-------------------------\n");
	ret = tgpio_get_cross_timestamp(dev, TGPIO_TIMER, TGPIO_ART, &local,
					&ref);
	handle_error(ret);
	printk("[%s] Timer: %d, Local Time: %08d:%08d, Ref. Time: %08d:%08d\n",
	       dev->name, TGPIO_TIMER, local.sec, local.nsec, ref.sec,
	       ref.nsec);

	/* Reset timer */
	ret = tgpio_port_set_time(dev, TGPIO_TIMER, INIT_TIME_SEC,
				  INIT_TIME_NSEC);
	handle_error(ret);
	ret = tgpio_port_adjust_frequency(dev, TGPIO_TIMER, 0);
	handle_error(ret);

	/*
	 * Loopback pins LOOPBACK_IN_PIN and LOOPBACK_OUT_PIN to test unlimited
	 * output and timebased input
	 *
	 */
	printk("\nTest for unlimited output and timebased input\n");
	printk("-----------------------------------------------\n");
	ret = tgpio_port_get_time(dev, TGPIO_TIMER, &start.sec, &start.nsec);
	handle_error(ret);
	start.sec++;
	interval.sec = REPEAT_SEC;
	interval.nsec = REPEAT_NSEC / 50; /* some random interval */
	ret = tgpio_pin_periodic_out(dev, LOOPBACK_OUT_PIN, TGPIO_TIMER, start,
				     interval, REPEAT_FOREVER, PWS);
	handle_error(ret);
	printk("[%s] Pin: %d, Timer: %d, Scheduled at:%08d:%08d\n",
	       dev->name, LOOPBACK_OUT_PIN, TGPIO_TIMER, start.sec,
	       start.nsec);
	ret = tgpio_port_get_time(dev, TGPIO_TIMER, &start.sec, &start.nsec);
	handle_error(ret);
	start.sec++;
	interval.sec = REPEAT_SEC;
	interval.nsec = REPEAT_NSEC / 10;
	ret = tgpio_pin_count_events(dev, LOOPBACK_IN_PIN, TGPIO_TIMER, start,
				     interval, TGPIO_EDGE, callback_handler);
	handle_error(ret);
	printk("[%s] Pin: %d, Timer: %d, Scheduled at:%08d:%08d\n",
	       dev->name, LOOPBACK_IN_PIN, TGPIO_TIMER, start.sec,
	       start.nsec);

	/*
	 * Loopback pins LOOPBACK_IN_PIN_1 and LOOPBACK_OUT_PIN_1 to test
	 * unlimited output/event ceiling input
	 *
	 */
	printk("\nTest for unlimited output and event ceiling input\n");
	printk("---------------------------------------------------\n");
	ret = tgpio_port_get_time(dev, TGPIO_TIMER, &start.sec, &start.nsec);
	handle_error(ret);
	start.sec++;
	interval.sec = REPEAT_SEC;
	interval.nsec = REPEAT_NSEC / 100;
	ret = tgpio_pin_periodic_out(dev, LOOPBACK_OUT_PIN_1, TGPIO_TIMER,
				     start, interval, REPEAT_FOREVER, PWS);
	handle_error(ret);
	printk("[%s] Pin: %d, Timer: %d, Scheduled at:%08d:%08d\n",
	       dev->name, LOOPBACK_OUT_PIN_1, TGPIO_TIMER, start.sec,
	       start.nsec);
	ret = tgpio_pin_timestamp_event(dev, LOOPBACK_IN_PIN_1, TGPIO_TIMER,
					EVENTS_CEILING, TGPIO_EDGE,
					callback_handler);
	handle_error(ret);
	printk("[%s] Pin: %d, Timer: %d, Scheduled at:%08d:%08d\n",
	       dev->name, LOOPBACK_IN_PIN_1, TGPIO_TIMER, start.sec,
	       start.nsec);

	printk("\n");
	while (1)
		k_sleep(K_MSEC(100));
}

/**
 * @}
 */

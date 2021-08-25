/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _PSE_
#include <stdint.h>
#include <common.h>
#include <device.h>
#include <thermal_framework.h>
#include "platform.h"
#ifdef _PSE_
#include <drivers/gpio-timed.h>
#endif
LOG_MODULE_REGISTER(eclite_tacho, CONFIG_ECLITE_LOG_LEVEL);

static APP_GLOBAL_VAR_BSS(1) uint8_t is_callbacked;
static APP_GLOBAL_VAR_BSS(1) uint8_t is_tacho_configured;
static APP_GLOBAL_VAR_BSS(1) int16_t rotation;
static APP_GLOBAL_VAR_BSS(1) uint64_t current_rotation;
static APP_GLOBAL_VAR_BSS(1) uint64_t previous_rotation;

#define TGPIO_TIMER_START_TIME_SEC              100
#define TGPIO_TIMER_START_TIME_NSEC             0
#define TGPIO_CALLBACK_INTERVAL_SEC             1
#define TGPIO_CALLBACK_INTERVAL_NSEC            0

#ifdef _PSE_
#define REPEAT_SEC      (0)
#define REPEAT_NSEC     (1000000U)

/*TGPIO_TMT_0*/
#define TGPIO_TIMER       TGPIO_TMT_0
#define TGPIO_RISING_EDGE 0
#define WRAP_AROUND       0xFFFFFFFFFFFFFFFF
#define ONE_MINUTE        60
#define PULSE_PER_REV     2

static void tgpio_callback(const struct device *port, uint32_t pin,
			   struct tgpio_time ts, uint64_t ev_cnt)
{
	ECLITE_LOG_DEBUG("Count: %llu", ev_cnt);

	is_callbacked = 1;
	previous_rotation = current_rotation;
	current_rotation = ev_cnt;

	rotation = current_rotation - previous_rotation;

	/* calculate rotation if count is rolled over.*/
	if (rotation < 0) {
		rotation = current_rotation - previous_rotation + WRAP_AROUND;
	}

	/* callback occurs at every second. converting to RPM.*/
	rotation = (rotation * ONE_MINUTE) / PULSE_PER_REV;
}

#endif /* _PSE_ */

struct device *eclite_get_tacho_device(const char *name)
{
	return (struct device *)device_get_binding(name);
}

int eclite_init_tacho(void *dev, void *cfg)
{
	struct tgpio_time start, interval;
	int ret;

	is_callbacked = false;
	is_tacho_configured = false;
	rotation = false;
	current_rotation = 0;
	previous_rotation = 0;
	tgpio_port_set_time(dev, TGPIO_TMT_0, TGPIO_TIMER_START_TIME_SEC,
			    TGPIO_TIMER_START_TIME_NSEC);

	tgpio_port_get_time(dev, TGPIO_TIMER, &start.sec,
			    &start.nsec);
	start.sec++;

	interval.sec = TGPIO_CALLBACK_INTERVAL_SEC;
	interval.nsec = TGPIO_CALLBACK_INTERVAL_NSEC;

	ret = tgpio_pin_count_events(dev, TGPIO_PIN, TGPIO_TIMER,
				     start, interval, TGPIO_RISING_EDGE,
				     tgpio_callback);
	if (ret) {
		LOG_ERR("TACHO init failed: %d", ret);
		return ret;
	}

	return 0;
}

int eclite_read_tacho(void *tgpio_dev, uint32_t *tacho)
{
	*tacho = rotation;
	return 0;
}

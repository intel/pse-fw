/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pwm.h>
#include <common.h>
#include <device.h>

struct device *eclite_get_pwm_device(const char *name)
{
	return (struct device *)device_get_binding(name);
}

int eclite_init_pwm(void *dev, void *cfg)
{
	/* Referene API like all other drivers */
	return 0;
}

int eclite_write_pwm(void *dev, uint32_t pwm, uint32_t period,
		     uint32_t pulse)
{
	pwm_flags_t flags = 0;

	return pwm_pin_set_cycles(dev, pwm, period, pulse, flags);
}

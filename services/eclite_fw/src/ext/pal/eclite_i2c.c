/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>
#include <stdint.h>
#include "eclite_hw_interface.h"
#include "common.h"

LOG_MODULE_REGISTER(eclite_i2c, CONFIG_ECLITE_LOG_LEVEL);

int eclite_i2c_config(void *dev, uint32_t cfg)
{
	const struct device *i2c_dev = (const struct device *)dev;

	return i2c_configure(i2c_dev, cfg);
}

struct device *eclite_get_i2c_device(const char *name)
{
	return (struct device *)device_get_binding(name);
}

int eclite_i2c_read_byte(void *ptr, uint16_t dev_addr, uint8_t reg_addr,
			 uint8_t *value)
{
	const struct device *ptr_device = (const struct device *)ptr;

	return i2c_reg_read_byte(ptr_device, dev_addr, reg_addr, value);
}

int eclite_i2c_write_byte(void *ptr, uint16_t dev_addr,
			  uint8_t reg_addr, uint8_t value)
{
	const struct device *ptr_device = (const struct device *)ptr;

	return i2c_reg_write_byte(ptr_device, dev_addr, reg_addr, value);

}

int eclite_i2c_read_word(void *ptr, uint16_t dev_addr,
			 uint8_t reg_addr, uint16_t *value)
{
	const struct device *ptr_device = (const struct device *)ptr;
	int ret;

	ret = i2c_burst_read(ptr_device, dev_addr, reg_addr,
			     (uint8_t *)value, 2);

	ECLITE_LOG_DEBUG(
		"I2C dev: %d error: %d, data:%d, slave:%d, reg:%d, len:2",
		(int)ptr_device, ret, *value, dev_addr, reg_addr);

	return ret;
}

int eclite_i2c_write_word(void *ptr, uint16_t dev_addr,
			  uint8_t reg_addr, uint16_t value)
{
	const struct device *ptr_device = (const struct device *)ptr;
	int ret;

	ret = i2c_burst_write(ptr_device, dev_addr, reg_addr,
			      (uint8_t *)&value, 2);

	ECLITE_LOG_DEBUG(
		"I2C dev: %d error: %d, data:%d, slave:%d, reg:%d, len:2",
		(int)ptr_device, ret, value, dev_addr, reg_addr);

	return ret;
}

int eclite_i2c_burst_read(void *ptr, uint16_t dev_addr,
			  uint8_t reg_addr, uint8_t *value, uint32_t len)
{
	const struct device *ptr_device = (const struct device *)ptr;

	return i2c_burst_read(ptr_device, dev_addr, reg_addr, value, len);
}

int eclite_i2c_burst_write(void *ptr, uint16_t dev_addr,
			   uint8_t reg_addr, uint8_t *value, uint32_t len)
{
	const struct device *ptr_device = (const struct device *)ptr;

	return i2c_burst_write(ptr_device, dev_addr, reg_addr, value, len);
}

int eclite_i2c_burst_read16(void *ptr, uint16_t dev_addr,
			    uint16_t reg_addr, uint8_t *value, uint32_t len)
{
	const struct device *ptr_device = (const struct device *)ptr;
	int ret;

	ret = i2c_write_read(ptr_device, dev_addr, &reg_addr, 2, value, len);

	ECLITE_LOG_DEBUG(
		"I2C dev: %d error: %d, data:%d, slave:%d, reg:%d, len:%u",
		(int)ptr_device, ret, *value, dev_addr, reg_addr, len);
	return ret;
}

int eclite_i2c_burst_write16(void *ptr, uint16_t dev_addr,
			     uint16_t reg_addr, uint8_t *value, uint32_t len)
{
	const struct device *ptr_device = (const struct device *)ptr;
	int ret;
	struct i2c_msg msg[2];

	msg[0].buf = (uint8_t *)&reg_addr;
	msg[0].len = 2U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)value;
	msg[1].len = len;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	ret = i2c_transfer(ptr_device, msg, 2, dev_addr);
	ECLITE_LOG_DEBUG(
		"I2C dev: %d error: %d, data:%d, slave:%d, reg:%d, len:%u",
		(int)ptr_device, ret, *value, dev_addr, reg_addr, len);

	return ret;
}


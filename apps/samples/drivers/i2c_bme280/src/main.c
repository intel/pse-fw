/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file
 * @brief Sample Application for I2C bus driver connect with BME280 sensor.
 * This sample shows how to use I2C to read/write from BME280 sensor.
 * The external sensor device is BKE280 chip. Detailed information for this
 * chip, you can see https://www.bosch-sensortec.com/products/environmental-
 * sensors/humidity-sensors-bme280/.
 * This sample read out BME280 chip id to show how to read a BME280 register.
 *
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more details
 * how to build the sample codes.
 *
 * @brief Hardware setup.
 * Please refer section 3.7.1 in “IntelPSE_SDK_User_Guide” for more
 * details for I2C hardware setup.
 * @{
 */

/* Local Includes */
#include <drivers/i2c.h>
#include <string.h>
#include <sys/printk.h>
#include <zephyr.h>

/* BME280 device slave address */
#define DEVICE_ADDR (0x76)
/* I2C master device used for this sample */
#define I2C_DEV_NAME ("I2C_1")
/* BME280 chip ID register */
#define BME_CHIP_ID_OFFSET (0xD0)

int main(int argc, char *argv[])
{
	int ret;
	uint8_t id = 0;

	/* Get the device handle for I2C device by 'I2C_DEV_NAME' */
	const struct device *i2c_dev = device_get_binding(I2C_DEV_NAME);
	/* I2C Module configuration
	 * I2C SPEED     : Fast speed (400K)
	 * MASTER OR SLAVE    : Master
	 */
	uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_MASTER;

	if (!i2c_dev) {
		printk("Cannot get I2C device\n");
		return -1;
	}
	printk("I2C: %s binding successful\n", I2C_DEV_NAME);

	if (i2c_configure(i2c_dev, i2c_cfg)) {
		printk("I2C config Fail.\n");
		return -1;
	}
	printk("I2C: %s config successful\n", I2C_DEV_NAME);

	/* Read data from BME280 */
	printk("Read chip ID from BME280...\n");

	ret = i2c_burst_read(i2c_dev, DEVICE_ADDR, BME_CHIP_ID_OFFSET, &id, 1);
	if (ret) {
		printk("I2C read Error %d.\n", ret);
		return ret;
	}
	printk("BME280 chip ID is 0x%x!\n", id);

	return 0;
}

/**
 * @}
 */

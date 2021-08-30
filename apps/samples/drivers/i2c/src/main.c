/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file
 * @brief Sample Application for I2C bus driver.
 * This sample shows how to use I2C to read/write from external memory device.
 * The external memory device is FM24V10 chip. Detailed information for this
 * chip, you can see https://www.cypress.com/file/41666/download.
 * This sample will first write 256 bytes data to FM24V10 address 0. And then
 * read 256 bytes data from FM24V10 address 0 to verify.
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

/* I2C Transfer buffer length */
#define BUF_LEN (256)
/* FM24V10 device slave address */
#define DEVICE_ADDR (0x50)
/* I2C master device used for this sample */
#define I2C_DEV_NAME ("I2C_0")
/* Data buffer for read */
static uint8_t rx_buff[BUF_LEN];
/* Data buffer for write */
static uint8_t tx_buff[BUF_LEN] = {0};
/* Used as FM24V10 internal address */
static uint16_t flash_reg_addr16;

int main(int argc, char *argv[])
{
	int ret, i, error = 0;
	struct i2c_msg msg[2];
	/* Get the device handle for I2C device by 'I2C_DEV_NAME' */
	const struct device *i2c_dev = device_get_binding(I2C_DEV_NAME);
	/* I2C Module configuration
	 * I2C SPEED     : Fast speed (400K)
	 * MASTER OR SLAVE    : Master
	 */
	uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_MASTER;

	memset(rx_buff, 0, sizeof(rx_buff));

	for (i = 0; i < BUF_LEN; i++) {
		tx_buff[i] = i;
	}

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

	/* Write data into FM24V10 */
	printk("Write data into FM24V10...\n");
	/* Set FM24V10 internal address you want to write here */
	flash_reg_addr16 = 0;
	msg[0].buf = (uint8_t *)(&flash_reg_addr16);
	msg[0].len = 2U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)tx_buff;
	msg[1].len = BUF_LEN;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	ret = i2c_transfer(i2c_dev, msg, 2, DEVICE_ADDR);
	if (ret) {
		printk("I2C write Error %d.\n", ret);
		return ret;
	}
	printk("I2C write finished!\n");

	/* Read data from FM24V10 */
	printk("Read data from FM24V10...\n");
	ret = i2c_write_read(i2c_dev, DEVICE_ADDR, &flash_reg_addr16, 2,
			     rx_buff, BUF_LEN);
	if (ret) {
		printk("I2C_Burst_Read Error %d.\n", ret);
		return ret;
	}
	printk("I2C read finished!\n");

	/* Check data */
	for (int i = 0; i < BUF_LEN; i++) {
		if (tx_buff[i] != rx_buff[i]) {
			printk("Read %d Value is: 0x%x, expected is: 0x%x\n", i,
			       rx_buff[i], tx_buff[i]);
			error++;
		}
	}

	if (error == 0) {
		printk("I2C write and read succeed!\n");
	} else {
		printk("I2C write and read failed!\n");
	}

	return 0;
}

/**
 * @}
 */

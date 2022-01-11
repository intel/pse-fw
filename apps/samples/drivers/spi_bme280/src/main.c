/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file
 * @brief Sample Application for SPI bus driver.
 * This sample shows how to use SPI to read/write from external sensor.
 * The external sensor device is BME280 chip. Detailed information for this
 * chip, you can see you can see https://www.bosch-sensortec.com/products/
 * environmental-sensors/humidity-sensors-bme280/.
 * This sample read out BME280 chip id to show how to read a BME280 register.
 *
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more details
 * how to build the sample codes.
 *
 * @brief Hardware setup.
 * Please refer section 3.3.1 in “IntelPSE_SDK_User_Guide” for
 * more details for SPI hardware setup.
 * @{
 */

/* Local Includes */
#include <drivers/spi.h>
#include <sys/printk.h>
#include <zephyr.h>

/* SPI master device used for this sample */
#define SPI_DEVICE ("SPI_0")
/* SPI mode 0 */
#define SPI_MODE_0 ((0 << 1) | (0 << 2))

#define BME_CHIP_ID_OFFSET (0xD0)

/* SPI Module configuration
 * SPI SPEED     : 2M
 * MASTER OR SLAVE    : Master
 * MODE: Mode 0
 * MSB OR LSB: MSB
 * FRAME SIZE: 8 bits
 * DATA LINE NUM: Single line
 */
struct spi_config spi_cfg_1 = {
	.frequency = 2000000,
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_0 | SPI_TRANSFER_MSB |
		     SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = BIT(0),
	.cs = NULL,
};

int main(int argc, char *argv[])
{
	/* Get the device handle for SPI device by 'SPI_DEVICE' */
	const struct device *dev = device_get_binding(SPI_DEVICE);
	int ret;
	uint8_t id = 0, addr = BME_CHIP_ID_OFFSET;

	const struct spi_buf tx_buf = {
		.buf = &addr,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf[2];
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2
	};

	if (!dev) {
		printk("Cannot get SPI device\n");
		return -1;
	}

	rx_buf[0].buf = NULL;
	rx_buf[0].len = 1;

	rx_buf[1].len = 1;
	rx_buf[1].buf = &id;

	printk("Start to read BME280 ID...\n");
	ret = spi_transceive(dev, &spi_cfg_1, &tx, &rx);
	if (ret) {
		printk("spi_transceive FAIL %d\n", ret);
		return ret;
	}

	printk("BME280 ID is 0x%x\n", id);

	return 0;
}

/**
 * @}
 */

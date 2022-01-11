/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file
 * @brief Sample Application for SPI bus driver.
 * This sample shows how to use SPI to read/write from external memory device.
 * The external memory device is MB85RS1MT chip. Detailed information for this
 * chip, you can see https://www.fujitsu.com/uk/Images/MB85RS1MT.pdf.
 * This sample will first write 128 bytes data to MB85RS1MT address 0. And then
 * read 128 bytes data from MB85RS1MT address 0 to verify.
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

/* SPI Transfer buffer length */
#define BUF_LEN (128)
/* SPI master device used for this sample */
#define SPI_DEVICE ("SPI_1")
/* SPI mode 0 */
#define SPI_MODE_0 ((0 << 1) | (0 << 2))
/* SPI receive buffer */
static uint8_t buffer_rx[BUF_LEN] = {};
/* Buffer for write enable command */
static uint8_t write_enable[] = { 0x06 };
/* Buffer for program flash */
static uint8_t write_mem[BUF_LEN] = { 0x02, 0, 0, 0 };
/* Buffer for read from flash */
static uint8_t read_mem[BUF_LEN] = { 0x03, 0,    0,    0,   0xff,
				     0xff, 0xff, 0xff, 0xff };

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
	int ret, i;
	int error = 0;

	/* Buffer for write enable command to flash */
	struct spi_buf flash_write_enable_cmd[] = {
		{
			.buf = write_enable,
			.len = 1,
		},

	};
	/* Buffer for write data command to flash */
	struct spi_buf flash_write_cmd[] = {
		{
			.buf = write_mem,
			.len = BUF_LEN,
		},
	};
	/* Buffer for receive data from flash */
	struct spi_buf data_bufs[] = {
		{
			.buf = buffer_rx,
			.len = BUF_LEN,
		},
	};
	/* Buffer for read data command to flash */
	struct spi_buf flash_read_cmd[] = {
		{
			.buf = read_mem,
			.len = BUF_LEN,
		},
	};
	/* Buffer set setup */
	struct spi_buf_set flash_write_enable = {
		.buffers = flash_write_enable_cmd, .count = 1
	};
	struct spi_buf_set rx = { .buffers = data_bufs, .count = 1 };
	struct spi_buf_set flash_write = { .buffers = flash_write_cmd,
					   .count = 1 };

	struct spi_buf_set flash_read = {
		.buffers = flash_read_cmd, .count = 1 };

	if (dev == NULL) {
		printk("No device!\n");
		return -1;
	}

	/* Init write buffer */
	for (i = 4; i < BUF_LEN; i++) {
		write_mem[i] = i;
	}

	printk("Write data to flash...\n");

	/* Write enable command */
	ret = spi_transceive(dev, &spi_cfg_1, &flash_write_enable, NULL);
	if (ret) {
		printk("Code %d while enable flash", ret);
		return ret;
	}

	/* Write flash */
	ret = spi_transceive(dev, &spi_cfg_1, &flash_write, NULL);
	if (ret) {
		printk("Code %d while program flash", ret);
		return ret;
	}

	/* Read flash */
	printk("Read data from flash...\n");
	ret = spi_transceive(dev, &spi_cfg_1, &flash_read, &rx);
	if (ret) {
		printk("Code %d while read flash", ret);
		return ret;
	}

	for (i = 4; i < BUF_LEN; i++) {
		if (buffer_rx[i] != write_mem[i]) {
			printk("Error data in %d\n", i);
			error++;
		}
	}

	for (i = 4; i < BUF_LEN; i++) {
		printk("%2d ", buffer_rx[i]);
	}

	if (error != 0) {
		printk("Flash program or read failed!\n");
	} else {
		printk("Flash program succeed!\n");
	}

	return 0;
}

/**
 * @}
 */

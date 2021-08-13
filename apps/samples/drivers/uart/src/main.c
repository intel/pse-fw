/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample Application for UART (Universal Asynchronous
 * Receiver/Transmitter) driver.
 * This example demonstrates how to use the UART for the following I/O
 * operations:
 *    • Buffered Polled Write
 *    • Buffered Polled Read
 *    • Asynchronous Write
 *    • Asynchronous Read
 * @{
 */

/**
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more details how to
 * build the sample codes.
 */

/**
 * @brief Hardware setup.
 * Please refer section 3.2.1 in “IntelPSE_SDK_User_Guide” for more details
 * for UART hardware setup.
 *
 * Note: Hardware connectivity differs when using Intel Add-In-Card(AIC).
 * Please refer to the relevant sections for uart connectivity in AIC user
 * manual when using Intel Add-In-Card.
 *
 */


/* Local Includes */
#include <sys/printk.h>
#include <string.h>
#include <drivers/uart.h>
#include <zephyr.h>

/** UART driver used for data transmit/receive. */
#define UART_DEV "UART_1"
/** Set max read length */
#define MAX_READ_LEN (64)
/** Set uart polled buffer length */
#define UART_POLLED_BUFF_LEN (5)
/** Set asynchronous data string */
#define ASYNC_DATA_STR "Sample Async Output for UART.\n"
/** Set size of asynchronous write length */
#define ASYNC_WRITE_LEN (sizeof(ASYNC_DATA_STR))
/** Set asynchronous read length */
#define ASYNC_READ_LEN (10)
/** Set timeout in micro seconds */
#define DEFAULT_TIMEOUT K_MSEC(30000)

/** define semaphore for data transmit */
static K_SEM_DEFINE(tx_sem, 0, 1);
/** define semaphore for data receive */
static K_SEM_DEFINE(rx_sem, 0, 1);
/** define buffer for asynchronous read , 32 byte aligned memory */
char rx_data_buff[ASYNC_READ_LEN] __aligned(32);
static void print_line_error(uint32_t line_err);

/* @breif uart callback write
 * callback function write operations
 */
static void uart_callback_write(const struct device *data, int status,
				uint32_t line_status, int len)
{
	printk("Write Data : len =%d  status =%d\n", len, status);
	if (status) {
		printk("Asynchronous Write failed\n\n");
	} else {
		printk("Asynchronous Write passed\n\n");
	}
	/* notify thread that data transmit is possible */
	k_sem_give(&tx_sem);
}

/* @breif uart callback read
 * callback function read operations
 */
static void uart_callback_read(const struct device *data, int status,
			       uint32_t line_status, int len)
{
	printk("Data Entered by User : %s\n", rx_data_buff);
	printk("Read Data : len =%d  status =%d\n", len, status);
	if (status) {
		printk("Asynchronous Read failed\n\n");
		print_line_error(line_status);
	} else {
		printk("Asynchronous Read passed\n\n");
	}
	/* notify thread that data receive is possible */
	k_sem_give(&rx_sem);
}

/* @brief buffered_polled_read
 * It blocks the call for reading data into a buffer in polled
 * mode for specified length.
 */
static int buffered_polled_read(const struct device *dev)
{
	uint32_t line_status;
	uint8_t buff[UART_POLLED_BUFF_LEN] __aligned(32);
	int ret;

	printk("Buffered polled read , input %d characters on test port\n",
	       UART_POLLED_BUFF_LEN);
	/* Read data over UART device(UART 1) for specified length */
	ret = uart_read_buffer_polled(dev, buff, UART_POLLED_BUFF_LEN,
				      &line_status);

	if (ret < 0) {
		printk("Buffered Polled Read failed\n");
		print_line_error(line_status);
		return -EIO;
	}

	printk("Data entered by User : %s\n", buff);
	printk("Buffered Polled Read passed\n\n");
	return 0;
}

/* @brief buffered_polled_write
 * It blocks the call for writing data from a buffer in polled
 * mode for specified length.
 */
static int buffered_polled_write(const struct device *dev)
{
	uint8_t buff[UART_POLLED_BUFF_LEN] __aligned(32) = { '1', '2', '3', '4',
							  '5' };

	int ret;

	printk("Buffered polled Mode on %s\n", dev->name);
	/* Write data over UART device(UART 1) for specified length */
	ret = uart_write_buffer_polled(dev, buff, UART_POLLED_BUFF_LEN);
	if (ret < 0) {
		printk("Buffered Polled Write failed\n");
		return -EIO;
	}
	printk("Buffered Polled Write passed\n\n");
	return 0;
}

/* @brief async_write
 * Does asynchronous write on specified uart device.
 */
static int async_write(const struct device *dev)
{
	int ret;

	printk("Asynchronous write on %s\n", dev->name);
	/* registered irq handlers for uart */
	uart_irq_callback_set(dev, NULL);

	/* performs asynchronous write on configured uart driver(UART 1) */
	ret = uart_write_async(dev, ASYNC_DATA_STR, ASYNC_WRITE_LEN,
			       uart_callback_write);
	if (ret) {
		printk("Async write failed ret=%d\n", ret);
		return ret;
	}

	/* notify thread that data transmit is not possible */
	ret = k_sem_take(&tx_sem, DEFAULT_TIMEOUT);
	if (ret) {
		printk("Async write timeout\n");
		return -EIO;
	}

	return 0;
}

/* @brief async_read
 * Does asynchronous read on specified uart device.
 */
static int async_read(const struct device *dev)
{
	int ret;

	printk("Asynchronous read on %s\n", dev->name);
	/* Register irq handlers for uart */
	uart_irq_callback_set(dev, NULL);

	/* Performs asynchronous read on configured uart driver(UART 1) */
	ret = uart_read_async(dev, rx_data_buff, ASYNC_READ_LEN,
			      uart_callback_read);

	if (ret) {
		printk("Async read failed ret=%d\n", ret);
		return ret;
	}

	printk("Waiting for %d characters on %s\n", ASYNC_READ_LEN,
	       dev->name);

	/* notify thread that data receive is not possible */
	ret = k_sem_take(&rx_sem, DEFAULT_TIMEOUT);
	if (ret) {
		printk("Async read timeout\n");
		return -EIO;
	}
	return 0;
}

/* @brief print line error
 * Prints UART line error message.
 */
static void print_line_error(uint32_t line_err)
{
	switch (line_err) {
	case 0:
		printk("No Line err\n");
		break;
	case UART_ERROR_OVERRUN:
		printk("UART_ERROR_OVERRUN\n");
		break;
	case UART_ERROR_PARITY:
		printk("UART_ERROR_PARITY\n");
		break;
	case UART_ERROR_FRAMING:
		printk("UART_ERROR_FRAMING\n");
		break;
	case UART_BREAK:
		printk("UART_ERROR_BREAK\n");
		break;
	default:
		printk("Unknown line err.\n");
	}
}

/* @brief print exit msg
 * Prints UART sample app exit message.
 */
static inline void print_exit_msg(void)
{
	printk("Exiting UART App.\n");
}

/* @brief main function
 * The UART application demonstrates the polled I/O operations
 * and the asynchronous I/O operations.
 */
void main(void)
{

	const struct device *uart_dev = NULL;

	printk("PSE UART Test Application\n");
	/* Get the device handle for UART device */
	uart_dev = device_get_binding(UART_DEV);

	if (!uart_dev) {
		printk("UART: Device driver not found\n");
		return;
	}

	printk("Testing Device %s\n\n", uart_dev->name);

	if (buffered_polled_write(uart_dev)) {
		printk("Buffered Polled Write failed");
		print_exit_msg();
		return;
	}

	if (buffered_polled_read(uart_dev)) {
		printk("Buffered Polled Read failed");
		print_exit_msg();
		return;
	}

	if (async_write(uart_dev)) {
		printk("Asynchronous Write failed\n");
		print_exit_msg();
		return;
	}

	if (async_read(uart_dev)) {
		printk("Asynchronous Read failed\n");
		print_exit_msg();
		return;
	}

	print_exit_msg();
}
/**
 * @}
 */

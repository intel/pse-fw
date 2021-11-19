/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @file
 * @brief Sample Application for PM (Power management)driver.
 * This sample application will demonstrate how you can use the power management
 * features .Here threads are created to enter into a sleep mode with a variable
 * delay. This will cause the Intel® PSE to enter into different power states:
 *     • D0i0 -->10 ms
 *     • D0i1 -->100 ms
 *     • D0i2 -->1200 ms
 *     • D0i3 -->5000 ms
 * 
 * These paramenters are controlled via dts entries. If an user wants to 
 * calibrate they are free to do so by editing the values in dts file.
 *
 * Enter command "sleep <d0ix>" on pse_shell to test different power states.
 * @{
 */

/**
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more details how to
 * build the sample codes.
 *
 */

/* Local Includes */
#include <kernel.h>
#include <logging/log.h>
#include <sys/printk.h>
#include "pm_service.h"
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <soc.h>
#include <stdlib.h>
#include <drivers/uart.h>
#include <zephyr.h>

/** Size of stack area used by thread */
#define STACK_SIZE 1024

/** Thread priority */
#define PRIO 0

/** Time dealy to enter into different power states in millisecond */
uint32_t m_delay = 10;

void main(void *dummy1, void *dummy2, void *dummy3);

/* creating  thread for main function */
K_THREAD_DEFINE(tid, STACK_SIZE, main, NULL, NULL, NULL, PRIO, 0, 0);

/* @brief sleep_func function
 * Sets device sleep time based on user input
 */
static int sleep_func(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);

	/** check if number arguments are less than 2 */
	if (argc < 2) {
		printk("Invlaid argument\n");
		return 0;
	}

	printk("user argument: %s, %d\n", argv[1], argc);
	/** For d0i0 power state, set sleep time to 10 millisecond */
	if (strcmp(argv[1], "d0i0") == 0) {
		m_delay = 10;
		/** For d0i1 power state, set sleep time to 100 millisecond */
	} else if (strcmp(argv[1], "d0i1") == 0) {
		m_delay = 100;
		/** For d0i2 power state, set sleep time to 1200 millisecond */
	} else if (strcmp(argv[1], "d0i2") == 0) {
		m_delay = 1200;
		/** For d0i3 power state, set sleep time to 5000 millisecond */
	} else if (strcmp(argv[1], "d0i3") == 0) {
		m_delay = 5000;
	}

	return 0;
}

/** Registers the sleep command */
SHELL_CMD_REGISTER(sleep, NULL, "Sleep D0ix", sleep_func);

/* @brief pm_task function
 * Put device into different power states based on input
 */
void pm_task(void *arg1, void *arg2, void *arg3)
{
	int i = 0;
	/** Pointers to UART device handler */
	const struct device *uart_dev = NULL;

	/** Get the device handle for UART device by 'uart_dev'. */
	uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	if (!uart_dev) {
		printk("Failed to get shell device binding\n");
	}

	printk("pm task %d\n", m_delay);

	/** Periodically enter and exit power state bases on m_delay */
	while (1) {
		printk("=====> %d\n", m_delay);
		/** Enable UART RX interrupt */
		uart_irq_rx_disable(uart_dev);
		k_sleep(K_MSEC(m_delay));
		/** Disable UART  RX interrupt */
		uart_irq_rx_enable(uart_dev);
		printk("<===== %d\n", m_delay);

		for (i = 0; i < 400; i++) {
			k_sleep(K_MSEC(4));
		}
	}
}

/* @brief main function
 * Performs PM operations
 */
void main(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);
}

/* creating  thread for pm_task function */
K_THREAD_DEFINE(my_tid, 1024, pm_task, NULL, NULL, NULL, 5, 0, 0);

/**
 * @}
 */

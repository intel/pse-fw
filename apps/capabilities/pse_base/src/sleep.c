/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Helper file for pse_base main application.
 * This contains the following functions:
 * control_shell_input()  -- Enable/disable UART IRQ on the basis
 * of shell state.
 * stop_pwm() -- Stop PWM Device
 * print_dev_list() -- Print list of active devices
 * update_sleep_delay() -- Updates the system idle state time
 * sleep_d0i0() -- Enter the D0i0 sleep state
 * sleep_d0i1() -- Enter the D0i1 sleep state
 * sleep_d0i2() -- Enter the D0i2 sleep state
 * sleep_d0i3() -- Enter the D0i3 sleep state
 * cmd_long_sleep() -- Long Sleep - Enter a sleep state (idle time)
 * for a given amount of time
 * cmd_stop_eclite_service() -- Stop Intel(R) EC-Lite Service
 * pm_task() -- power management task to enter a D0ix sleep state
 * @{
 */

/* Local Includes */
#include <kernel.h>
#include <logging/log.h>
#include <pm_service.h>
#include <sys/printk.h>
#include <drivers/pwm.h>
#include <sedi.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <soc.h>
#include <stdlib.h>
#include <string.h>
#include <drivers/uart.h>
#include <zephyr.h>

#define SLEEP_TIME K_MESC(30)
/* PWM device name */
#define PWM_1_NAME "PWM_1"
static const struct pm_state_info residency_info[] =
	PM_STATE_INFO_DT_ITEMS_LIST(DT_NODELABEL(cpu0));
static size_t residency_info_len = PM_STATE_DT_ITEMS_LEN(DT_NODELABEL(cpu0));


/* Idle time */
static uint32_t m_delay;
/* Pointer to UART device handler */
static const struct device *uart_dev;
/* Shell state */
static bool shell_active = true;

#ifdef CONFIG_ECLITE_SERVICE
extern void eclite_d0ix(uint8_t state);
#endif

/* @brief control shell input
 * Enable/disable UART IRQ on the basis of
 * shell state.
 */
void control_shell_input(bool enable)
{
	if (shell_active) {
		if (enable) {
			uart_irq_rx_enable(uart_dev);
			uart_irq_tx_enable(uart_dev);
		} else {
			uart_irq_rx_disable(uart_dev);
			uart_irq_tx_disable(uart_dev);
		}
	}
}

/* @brief stop PWM
 * Stop PWM device.
 */
#ifdef CONFIG_ECLITE_SERVICE
void stop_pwm(void)
{
	const struct device *pwm_1;

	/* Get the device handler for PWM device. */
	pwm_1 = device_get_binding(PWM_1_NAME);
	if (!pwm_1) {
		printk("%s - Bind failed\n", PWM_1_NAME);
		return;
	}
	pwm_pin_set_cycles(pwm_1, 7, 0xff, 0, 0);
}
#endif

/* @brief print device list
 * Print list of active devices
 */
static int print_dev_list(void)
{
	const struct device *pm_device_list;
	int count = 0, i;

	/* Get status of Atom owned PSE IO devices. */
	if (get_atom_dev_status() != 0) {
		printk("All Atom owned PSE IO devices are not idle\n");
		printk("Deferring D0ix entry !!!\n");
		return -1;
	}

	/* Get status of PSE IO devices. */
	if (pm_device_is_any_busy() != 0) {
		printk("All PSE IO devices are not idle, deferring D0ix entry "
		       "!!!\n");
		z_device_get_all_static(&pm_device_list);

		printk("Following device are not idle!!\n");
		for (i = 0; i < count; i++) {
			if (pm_device_is_busy(&pm_device_list[i])) {

				printk("%s\n", pm_device_list[i].name);
			}
		}
		return -1;
	}
	return 0;
}

/* @brief update sleep delay
 * Updates how much time to put the
 * system in idle state for.
 */
void update_sleep_delay(int delay)
{
	int ret;

	control_shell_input(false);
	ret = print_dev_list();
	/* Update sleep time if all devices idle. */
	if (ret == 0) {
		m_delay = delay;
	} else {
		control_shell_input(true);
	}
}

/* @brief sleep D0i0
 * Enter D0i0 sleep state
 */
static int sleep_d0i0(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t delay = 0;

	/* Use ARG_UNUSED([arg]) to avoid compiler
	 * warnings about unused arguments
	 */
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (residency_info_len) {
		delay = ((residency_info[0].min_residency_us +
			  residency_info[0].min_residency_us / 2) / 1000);
		m_delay = delay;
		update_sleep_delay(delay);
	}
	return 0;
}

/* @brief sleep D0i1
 * Enter D0i1 sleep state
 */
static int sleep_d0i1(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t delay = 0;

	/* Use ARG_UNUSED([arg]) to avoid compiler
	 * warnings about unused arguments
	 */
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (residency_info_len < 2) {
		printk("Not supported\n");
	} else {
		delay = ((residency_info[1].min_residency_us +
			  residency_info[1].min_residency_us / 2) / 1000);
		m_delay = delay;
		update_sleep_delay(delay);
	}
	return 0;
}

/* @brief sleep D0i2
 * Enter D0i2 sleep state
 */
static int sleep_d0i2(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t delay = 0;

	/* Use ARG_UNUSED([arg]) to avoid compiler
	 * warnings about unused arguments
	 */
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (residency_info_len < 3) {
		printk("Not supported\n");
	} else {
		delay = ((residency_info[2].min_residency_us +
			  residency_info[2].min_residency_us / 2) / 1000);
		m_delay = delay;
		update_sleep_delay(delay);
	}
	return 0;
}

/* @brief sleep D0i3
 * Enter D0i3 sleep state
 */
static int sleep_d0i3(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t delay = 0;

	/* Use ARG_UNUSED([arg]) to avoid compiler
	 * warnings about unused arguments
	 */
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (residency_info_len < 4) {
		printk("Not supported\n");
	} else {
		delay = ((residency_info[3].min_residency_us +
			  residency_info[3].min_residency_us / 2) / 1000);
		m_delay = delay;
		update_sleep_delay(delay);
	}
	return 0;
}

/* @brief long sleep
 * Enter sleep state for time given by
 * command line argument.
 */
static int cmd_long_sleep(const struct shell *shell, size_t argc, char **argv)
{
	int sleep_time = 0;

	/* Use ARG_UNUSED([arg]) to avoid compiler
	 * warnings about unused arguments
	 */
	ARG_UNUSED(shell);

	if (argc > 1) {
		sleep_time = atoi(argv[1]);
		sleep_time = sleep_time * 1000 * 60;
		update_sleep_delay(sleep_time);
	} else {
		printk(
			"Invalid arguments: long_sleep <sleep time in minutes>\n");
	}
	return 0;
}

/* @brief stop ECLite service
 * Stop ECLite service activity.
 */
#ifdef CONFIG_ECLITE_SERVICE
static int cmd_stop_eclite_service(const struct shell *shell, size_t argc,
				   char **argv)
{
	/* Use ARG_UNUSED([arg]) to avoid compiler
	 * warnings about unused arguments
	 */
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);

	/* Stop PWM device before disabling ECLite service */
	stop_pwm();
	eclite_d0ix(1);
	return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(
	d0ix, SHELL_CMD(d0i0, NULL, "Enter D0i0 sleep state", sleep_d0i0),
	SHELL_CMD(d0i1, NULL, "Enter D0i1 sleep state", sleep_d0i1),
	SHELL_CMD(d0i2, NULL, "Enter D0i2 sleep state", sleep_d0i2),
	SHELL_CMD(d0i3, NULL, "Enter D0i3 sleep state", sleep_d0i3),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(sleep, &d0ix, "Sleep D0ix", NULL);
SHELL_CMD_REGISTER(long_sleep, NULL, "Sleep for inputted minutes",
		   cmd_long_sleep);

#ifdef CONFIG_ECLITE_SERVICE
SHELL_CMD_REGISTER(stop_eclite_service, NULL, "Stop EClite service",
		   cmd_stop_eclite_service);
#endif

/* @brief power management task
 * Enter D0ix sleep state
 */
void pm_task(void *arg1, void *arg2, void *arg3)
{
	/* Use ARG_UNUSED([arg]) to avoid compiler
	 * warnings about unused arguments
	 */
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret = 0;

	/* Get the device handler for UART device. */
	uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	if (!uart_dev) {
		printk("Failed to get shell device binding\n");
		return;
	}

	/* Determine shell is activated or not. */
	ret = sedi_get_config(SEDI_CONFIG_SHELL_EN, NULL);
	if (ret != SEDI_CONFIG_SET) {
		printk("SHELL is deactivated\n");
		uart_irq_rx_disable(uart_dev);
		shell_active = false;
		return;
	}

	/* Enter D0ix state. */
	while (1) {
		if (m_delay > 0) {
			printk("\n\n ----- Enter sleep %d -----\n\n", m_delay);

			k_sleep(K_MSEC(m_delay));
			control_shell_input(true);

			printk("\n\n ----- Exit sleep %d-----\n\n", m_delay);
			m_delay = 0;
		}

		k_busy_wait(1000);
	}
}

K_THREAD_DEFINE(my_tid, 1024, pm_task, NULL, NULL, NULL, 30, 0, 0);

/**
 * @}
 */

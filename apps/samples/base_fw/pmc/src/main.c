/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/*
 * @file
 * @brief Sample Application for PMC service.
 * This application tests PMC service for intel_pse target and demonstrates
 * how to communicate securely with the PMC subsystem and
 * issue critical platform control functions (example, power down, restart,
 * and wake up from Sx state)
 * @{
 */

/*
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more details how to
 * build the sample codes.
 */

/*
 * @brief Hardware setup.
 * There is no specific external hardware setup required for the PMC
 * communication sample application.
 */

/* Local Includes */
#include <kernel_structs.h>
#include <sys/printk.h>
#include "pmc_service.h"
#include "sedi.h"
#include <drivers/sideband.h>
#include <string.h>
#include <drivers/uart.h>
#include <user_app_framework/user_app_framework.h>
#include <zephyr.h>

/* PECI commands */
#define PMC_PECI_GET_TEMP_CMD 0x12
#define PECI_TARGET_ADDRESS 0x30
#define PECI_GET_TEMP_CMD 0x01
#define PECI_GET_TEMP_RSVD 0x0
#define PECI_GET_TEMP_WR_LEN 0x1
#define PECI_GET_TEMP_RD_LEN 0x2

#define PMC_POWER_STATE_CHANGE_CMD 0x14
/** PMC message payload to shutdown the host */
#define PMC_HOST_SHUTDOWN 16
/** PMC message payload to reset the host */
#define PMC_HOST_RESET 4
/** PMC message payload to wake the host */
#define PMC_HOST_WAKE_CODE  32

/** UART device name */
#define UART_DEV "UART_2"

/** timeout value for Sx notification */
#define SX_NOTIFY_TIMEOUT 4000

/** Declare a global varible which only this App(3) can access. */
APP_GLOBAL_VAR(3) static bool sx_entry;

/** semaphore to synchronize the Sx events */
K_SEM_DEFINE(sx_rcv_evnt, 0, 1);

static const void *obj_list[] = { &sx_rcv_evnt };

struct get_temp_req_t {
	uint16_t cmd;
	uint16_t rsvd;
	uint8_t peci_adr;
	uint8_t wr_len;
	uint8_t rd_len;
	uint8_t peci_cmd;
};

struct get_temp_res_t {
	uint16_t cmd;
	uint16_t rsvd;
	uint8_t peci_res;
	uint8_t res_lsb;
	uint8_t res_msb;
};

/* @brief Sx callback.
 * Call back function invoked following
 * a Sx event
 */
void sx_callback(sedi_pm_sx_event_t event, void *ctx)
{
	switch (event) {
	case PM_EVENT_HOST_SX_ENTRY:
		k_sem_give(&sx_rcv_evnt);
		sx_entry = true;
		printk("Sx Entry\n");
		break;
	case PM_EVENT_HOST_SX_EXIT:
		k_sem_give(&sx_rcv_evnt);
		sx_entry = false;
		printk("Sx Exit\n");
		break;
	default:
		printk("Invalid\n");
		break;
	}
}

/* @brief Get host temperature.
 * This function demonstrates how to retrieve
 * the host temperaute using the long format
 * of peci command.
 */
static void get_host_temp(void)
{
	struct pmc_msg_t long_pmc_msg;
	struct get_temp_res_t *res;

	memset(&long_pmc_msg, 0, sizeof(long_pmc_msg));

	long_pmc_msg.client_id = PMC_CLIENT_ECLITE;
	long_pmc_msg.format = FORMAT_LONG;
	long_pmc_msg.wait_for_ack = PMC_WAIT_ACK;
	long_pmc_msg.msg_size = 12;

	struct get_temp_req_t get_temp = {
		.cmd = PMC_PECI_GET_TEMP_CMD,
		.rsvd = PECI_GET_TEMP_RSVD,
		.peci_adr = PECI_TARGET_ADDRESS,
		.wr_len = PECI_GET_TEMP_WR_LEN,
		.rd_len = PECI_GET_TEMP_RD_LEN,
		.peci_cmd = PECI_GET_TEMP_CMD,
	};

	memcpy(&long_pmc_msg.u.msg, &get_temp, sizeof(struct get_temp_req_t));

	if (pmc_sync_send_msg(&long_pmc_msg) != 0) {
		printk("Error in pmc request\n");
	}

	res = (struct get_temp_res_t *)(&long_pmc_msg.u.msg);

	printk("LSB CPU Temp: %d\n", res->res_lsb);
	printk("MSB CPU Temp: %d\n", res->res_msb);
}

/* @brief shutdown host.
 * This function is used to shutdown
 * the host using short message format of
 * peci command.
 */
static void shutdown_host(void)
{
	struct pmc_msg_t short_pmc_msg = { 0, };

	short_pmc_msg.client_id = PMC_CLIENT_ECLITE;
	short_pmc_msg.format = FORMAT_SHORT;
	short_pmc_msg.u.short_msg.cmd_id = PMC_POWER_STATE_CHANGE_CMD;
	short_pmc_msg.u.short_msg.payload = PMC_HOST_SHUTDOWN;
	short_pmc_msg.msg_size = 0;

	printk("Sending PMC short msg to shutdown\n");

	if (pmc_sync_send_msg(&short_pmc_msg) != 0) {
		printk("Error in pmc request\n");
	}
}

/* @brief reset host.
 * This function is used to reset
 * the host using short message format of
 * peci command.
 */
static void reset_host(void)
{
	struct pmc_msg_t short_pmc_msg = { 0, };

	short_pmc_msg.client_id = PMC_CLIENT_ECLITE;
	short_pmc_msg.format = FORMAT_SHORT;
	short_pmc_msg.u.short_msg.cmd_id = PMC_POWER_STATE_CHANGE_CMD;
	short_pmc_msg.u.short_msg.payload = PMC_HOST_RESET;

	printk("Sending PMC short msg to reset\n");

	if (pmc_sync_send_msg(&short_pmc_msg) != 0) {
		printk("Error in pmc request\n");
	}
}

/* @brief power up host.
 * This function is used to power up the host using
 * raw sideband command.
 */
static void power_up_host(void)
{
	struct pmc_msg_t short_pmc_msg = { 0, };

	short_pmc_msg.client_id = PMC_CLIENT_ECLITE;
	short_pmc_msg.format = FORMAT_SHORT;
	short_pmc_msg.u.short_msg.cmd_id = PMC_POWER_STATE_CHANGE_CMD;
	short_pmc_msg.u.short_msg.payload = PMC_HOST_WAKE_CODE;

	printk("Sending PMC Message for Power up\n");
	if (pmc_sync_send_msg(&short_pmc_msg) != 0) {
		printk("Error in pmc request\n");
	}
}

/* @brief assert host
 * This function is used to wake up the host
 * by sending PME_ASSERT
 */
static void pme_assert_host(void)
{
	struct pmc_msg_t hwsb_pmc_msg = { 0, };

	hwsb_pmc_msg.format = FORMAT_HWSB_PME_REQ;

	printk("Sending HW PME_ASSERT to wake up host.\n");
	if (pmc_sync_send_msg(&hwsb_pmc_msg) != 0) {
		printk("Error in pmc request\n");
	}
}

/* @brief app config
 * This function specifies the required configurations
 * for this application
 */
static void app_config(void)
{
	const struct device *uart_dev;

	uart_dev = device_get_binding(UART_DEV);

	/* For OOB usecase GBE driver is always busy,
	 * hence setting uart driver busy here.
	 */
	pm_device_busy_set(uart_dev);

	sedi_pm_register_sx_notification(sx_callback, NULL);
	sedi_pm_set_control(SEDI_PM_IOCTL_OOB_SUPPORT, 1);
}

/* @brief app_main function
 * The sample app demonstrates how to communicate to the
 * PMC subsystem securely and issue platform control
 * functionalities such as power down, restart and wakeup
 * from Sx state and also demonstrates how to use the
 * peci short and long format commands.
 */
static void app_main(void *p1, void *p2, void *p3)
{
	const struct device *uart_dev;
	char action;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("PSE PMC App %s\n", CONFIG_BOARD);

	k_sleep(K_MSEC(50));

	k_thread_system_pool_assign(k_current_get());

	uart_dev = device_get_binding(UART_DEV);

	if (!uart_dev) {
		printk("UART: Device driver not found\n");
		return;
	}

	/* Polled Mode on UART_2*/

	while (1) {
		printk("Enter 1 to Shutdown\n");
		printk("Enter 2 to Reset\n");
		printk("Enter 3 to Power up\n");
		printk("Enter 4 to Get Host Temperature\n");
		printk("Enter 5 to Trigger PME assert (hostwakeup)\n");

		while (uart_poll_in(uart_dev, &action) < 0) {
			k_sleep(K_MSEC(1));
		}

		switch (action) {
		case '1':
			if (sx_entry) {
				printk("Host in Sx state.\n");
			} else {
				shutdown_host();
				k_sem_take(&sx_rcv_evnt, K_MSEC(SX_NOTIFY_TIMEOUT));
			}
			break;
		case '2':
			reset_host();
			break;
		case '3':
			power_up_host();
			break;
		case '4':
			get_host_temp();
			break;
		case '5':
			if (!sx_entry) {
				printk("Host is in S0 state.\n");
			} else {
				pme_assert_host();
				k_sem_take(&sx_rcv_evnt, K_MSEC(SX_NOTIFY_TIMEOUT));
			}
			break;
		default:
			printk("Invalid Input\n");
			break;
		}
	}
}

/* Defining the pmc app in user mode with app-id 3 */
DEFINE_USER_MODE_APP(3, USER_MODE_APP | K_PART_GLOBAL, app_main, 2048,
		     (void **)obj_list, 1, app_config);

/**
 * @}
 */

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "eclite_dispatcher.h"
#include "platform.h"
#include "eclite_device.h"
#include "eclite_hostcomm.h"
#include "eclite_hw_interface.h"
#include "charger_framework.h"
#include "eclite_opregion.h"
#include "thermal_framework.h"
#include <sedi.h>

LOG_MODULE_REGISTER(dispatcher, CONFIG_ECLITE_LOG_LEVEL);

/*Time duration and period define are in milliseconds.*/
#define MAX_PERCENTAGE  100
#define ECLITE_TASK_STACK_SIZE  1600
#define ECLITE_TASK_PRIORITY    K_PRIO_PREEMPT(0)
#define ECLITE_QUEUE_LEN        sizeof(struct dispatcher_queue_data)
#define ECLITE_QUEUE_DEPTH      20
#define ECLITE_POOL_MIN_SIZE_BLOCK 8
#define ECLITE_POOL_MAX_SIZE_BLOCK 128
#define ECLITE_POOL_MAX_BLOCKS 4
#define ECLITE_POOL_BUF_ALIGN_VALUE 4

#define UCSI_MSG_OUT_CONTROL 24 /* MSG OUT & CONTROL Struct size */
#define UCSI_MSG_IN_CCI 20      /* MSG IN & CCI Struct size */

__kernel struct k_thread dispatcher_task;
K_MSGQ_DEFINE(dispatcher_queue, ECLITE_QUEUE_LEN, ECLITE_QUEUE_DEPTH, 4);
K_THREAD_STACK_DEFINE(dispatcher_stack, 1600);
K_HEAP_DEFINE(ecl_pool_name,
	      ECLITE_POOL_MAX_SIZE_BLOCK * ECLITE_POOL_MAX_BLOCKS);

static void eclite_dispatcher(void *p1, void *p2, void *p3);
static int gpio_event_processing(struct eclite_device
				 *platform_gpio[], uint32_t gpio_number);

void timer_periodic_callback(struct k_timer *timer)
{
	struct dispatcher_queue_data event_data;


	event_data.event_type = TIMER_EVENT;
	eclite_post_dispatcher_event(&event_data);
	ECLITE_LOG_DEBUG("Timer service executed");
}
void timer_stop_callback(struct k_timer *timer)
{
	ECLITE_LOG_DEBUG("EcliteTimer Stopped");
}

int init_drivers(void)
{
	for (int dev_count = 0; dev_count < no_of_devices; dev_count++) {
		if (DEV_OPS_SUCCESS !=
		    init_device(platform_devices[dev_count]->device_typ)) {
			LOG_ERR("Error init device: %d", dev_count);
			return ERROR;
		}
	}
	return 0;
}

int init_kernel_services(void)
{
	/* create User mode thread */
	k_thread_create(&dispatcher_task, dispatcher_stack,
			ECLITE_TASK_STACK_SIZE, eclite_dispatcher, NULL,
			NULL, NULL, ECLITE_TASK_PRIORITY,
			APP_USER_MODE_TASK | K_INHERIT_PERMS, K_FOREVER);

#ifndef CONFIG_USERSPACE
	k_thread_heap_assign(&dispatcher_task, &ecl_pool_name);
#endif

	k_thread_start(&dispatcher_task);
	return 0;
}

void handle_cold_plug(void)
{
	uint8_t data = FG_EVENT;
	uint16_t charger_status = 0;

	charging_manager_callback(data, &charger_status);
	update_opregion(DEV_FG);
}

int dispatcher_init(void)
{
	int ret;

#ifdef CONFIG_ECLITE_CHARGING_FRAMEWORK
	handle_cold_plug();
#endif

	ret = init_host_communication();
	if (ret) {
		LOG_ERR("Host communication init failure. ret: %d", ret);
		return ERROR;
	}
	ret = init_kernel_services();
	if (ret) {
		LOG_ERR("Tasks init failure. ret: %d", ret);
		return ERROR;
	}
	return 0;
}

static int power_state_change(uint8_t notification)
{
	int ret;

	ret = pmc_command(SEND_POWER_STATE_CHANGE, &notification);
	if (ret) {
		LOG_ERR("Invalid pmc notification message. ret: %d", ret);
		return ret;
	}

	return 0;
}

static void handle_low_battery_wake(struct eclite_opregion_t *opregion)
{
	uint16_t capacity = opregion->battery_info.full_charge_capacity;
	uint16_t rem_capacity_percent =
		opregion->battery_info.remaining_capacity *
		MAX_PERCENTAGE / capacity;
	uint16_t state = opregion->battery_info.state;
	uint16_t crit_bat_level = CONFIG_ECLITE_BATTERY_CRITICAL_LEVEL;

	ECLITE_LOG_DEBUG("Battery charge now: %d%%, Charging: %s, Sx state: %d",
			 rem_capacity_percent,
			 (state & BATTERY_CHARGING) ? "Yes" : "No",
			 eclite_sx_state);
	/* Critical battery, wake up if in s3 */
	if ((state & BATTERY_DISCHARGING) &&
	    (rem_capacity_percent < crit_bat_level) &&
	    (eclite_sx_state == PM_RESET_TYPE_S3)) {
		ECLITE_LOG_DEBUG("Battery critical level, Wake up host");
		power_state_change(PMC_SRT_WAKEUP);
	}
}

static void handle_crit_battery_shutdown(struct eclite_opregion_t *opregion)
{
	uint16_t capacity = opregion->battery_info.full_charge_capacity;
	uint16_t critical_level = opregion->battery_info.critical_level;
	/* equivalent of 1% capacity*/
	uint16_t delta = opregion->battery_info.design_capacity /
			 MAX_PERCENTAGE;

	if (capacity < critical_level) {
		if (capacity < critical_level - delta) {
			power_state_change(PMC_SRT_SHUTDOWN);
		}
	}
}

static void handle_crit_thermal_shutdown(struct eclite_opregion_t *opregion)
{
	int16_t cpu_temp, skin_temp[4];

	cpu_temp = (int16_t)opregion->cpu_temperature;
	skin_temp[0] = (int16_t)opregion->systherm0_temp1;
	skin_temp[1] = (int16_t)opregion->systherm1_temp1;
	skin_temp[2] = (int16_t)opregion->systherm2_temp1;
	skin_temp[3] = (int16_t)opregion->systherm3_temp1;

#ifdef CONFIG_THERMAL_SHUTDOWN_ENABLE
	if (cpu_temp > TEMP_CPU_CRIT_SHUTDOWN) {
		LOG_ERR("CPU [%d] is too hot, shutting down",
			opregion->cpu_temperature);
		power_state_change(PMC_SRT_SHUTDOWN);
	}
#endif
	if ((skin_temp[0] > TEMP_SYS_CRIT_SHUTDOWN) ||
	    (skin_temp[1] > TEMP_SYS_CRIT_SHUTDOWN) ||
	    (skin_temp[2] > TEMP_SYS_CRIT_SHUTDOWN) ||
	    (skin_temp[3] > TEMP_SYS_CRIT_SHUTDOWN)) {
		LOG_ERR("Platform sensor too hot, shutting down\n");
		power_state_change(PMC_SRT_SHUTDOWN);
	}

}

static inline void eclite_check_thermal_event(uint16_t sensor_condition,
					      uint16_t host_event_id)
{
	if (sensor_condition) {
		eclite_send_event(host_event_id);
	}
}

static void eclite_send_thermal_event(uint16_t *thermal_status)
{
	eclite_check_thermal_event(*thermal_status & BIT0,
				   ECLITE_EVENT_SYSTEMP0_ALERT);

	eclite_check_thermal_event(*thermal_status & BIT1,
				   ECLITE_EVENT_SYSTEMP1_ALERT);

	eclite_check_thermal_event(*thermal_status & BIT2,
				   ECLITE_EVENT_SYSTEMP2_ALERT);

	eclite_check_thermal_event(*thermal_status & BIT3,
				   ECLITE_EVENT_SYSTEMP3_ALERT);

	eclite_check_thermal_event(*thermal_status & BIT(CPU),
				   ECLITE_EVENT_CPUTEMP_UPDATE);
}

static void eclite_send_charger_event(uint16_t *charger_status)
{
	/* charger status change.*/
	if (*charger_status & BIT1) {
		if (*charger_status & BIT0) {
			eclite_send_event(ECLITE_EVENT_CHARGER_CONNECT);
		} else {
			eclite_send_event(ECLITE_EVENT_CHARGER_DISCONNECT);
		}
	}
}

static void process_charger_command(uint8_t event)
{
	struct charger_framework_data cmd;

	cmd.event_id = event;
	switch (event) {
	case ECLITE_OS_EVENT_BTP_UPDATE:
		cmd.data = (int) eclite_opregion.battery_info.trip_point[0];
		ECLITE_LOG_DEBUG("BTP = %d",
				 eclite_opregion.battery_info.trip_point[0]);
		break;
	default:
		ECLITE_LOG_DEBUG("Wrong event = %d\n", event);
	}
	charger_framework_command(&cmd);
}

static void process_thermal_command(uint8_t event)
{
	struct thermal_command_data cmd;

	cmd.event_id = event;
	switch (event) {
	case ECLITE_OS_EVENT_PWM_UPDATE:
		cmd.data[0] = eclite_opregion.pwm_dutycyle;
		break;
	case ECLITE_OS_EVENT_THERM_ALERT_TH_UPDATE:
		cmd.data[0] = eclite_opregion.sys0_alert3_1;
		cmd.data[1] = eclite_opregion.sys0_alert1_1;

		cmd.data[2] = eclite_opregion.sys1_alert3_1;
		cmd.data[3] = eclite_opregion.sys1_alert1_1;

		cmd.data[4] = eclite_opregion.sys2_alert3_1;
		cmd.data[5] = eclite_opregion.sys2_alert1_1;

		cmd.data[6] = eclite_opregion.sys3_alert3_1;
		cmd.data[7] = eclite_opregion.sys3_alert1_1;
		break;
	case ECLITE_OS_EVENT_CRIT_TEMP_UPDATE:
		cmd.data[0] = eclite_opregion.sys0_crittemp_thr1;
		cmd.data[1] = eclite_opregion.sys1_crittemp_thr1;
		cmd.data[2] = eclite_opregion.sys2_crittemp_thr1;
		cmd.data[3] = eclite_opregion.sys3_crittemp_thr1;
		break;

	default:
		ECLITE_LOG_DEBUG("Wrong event: %d", event);
		break;
	}
	thermal_command(&cmd);
}

static void process_ucsi_command(uint8_t event)
{
	struct ucsi_info_in data;
	uint8_t buf[UCSI_MSG_OUT_CONTROL];

	memset(&data, 0, sizeof(data));

	switch (event) {
	case ECLITE_OS_EVENT_UCSI_UPDATE:
		memcpy(&data.msg_out_info,
		       &eclite_opregion.ucsi_in_data.msg_out_info,
		       sizeof(data.msg_out_info));
		memcpy(&data.control_info,
		       &eclite_opregion.ucsi_in_data.control_info,
		       sizeof(data.control_info));
		memcpy(&buf, &data, UCSI_MSG_OUT_CONTROL);
		ECLITE_LOG_DEBUG("control_info.cmd: %d, datalen :%d",
			eclite_opregion.ucsi_in_data.control_info.command,
			eclite_opregion.ucsi_in_data.control_info.dataLen);
		ucsi_command(USBC_MSG_OUT, UCSI_MSG_OUT_CONTROL, buf);
		break;
	default:
		ECLITE_LOG_DEBUG("Wrong event = %d", event);
		break;
	}
	ucsi_framework(USBC_MSG_IN);
}

static void eclite_send_ucsi_event(void)
{
	uint8_t buf[UCSI_MSG_IN_CCI];

	ucsi_command(USBC_MSG_IN, UCSI_MSG_IN_CCI, buf);
	if (buf != NULL) {
		memcpy(&eclite_opregion.ucsi_in_data.msg_in_info, buf,
		       sizeof(eclite_opregion.ucsi_in_data.msg_in_info));
		memcpy(&eclite_opregion.ucsi_in_data.cci_info,
		       buf + sizeof(eclite_opregion.ucsi_in_data.msg_in_info),
		       sizeof(eclite_opregion.ucsi_in_data.cci_info));
	}

	ECLITE_LOG_DEBUG("cci_info.busy: %x",
			 eclite_opregion.ucsi_in_data.cci_info.busy);
	ECLITE_LOG_DEBUG("ackcmdcomp: %x",
			 eclite_opregion.ucsi_in_data.cci_info.ackcmdcomp);
	ECLITE_LOG_DEBUG("cci: %x",
			 eclite_opregion.ucsi_in_data.cci_info.cci);
	ECLITE_LOG_DEBUG("cmdcompleted: %x",
			 eclite_opregion.ucsi_in_data.cci_info.cmdcompleted);
	ECLITE_LOG_DEBUG("cancelcompleted: %x",
			 eclite_opregion.ucsi_in_data.cci_info.cancelcompleted);
	ECLITE_LOG_DEBUG("resetcompleted: %x",
			 eclite_opregion.ucsi_in_data.cci_info.resetcompleted);
	ECLITE_LOG_DEBUG("notsupported: %x",
			 eclite_opregion.ucsi_in_data.cci_info.notsupported);
	ECLITE_LOG_DEBUG("error: %x",
			 eclite_opregion.ucsi_in_data.cci_info.error);
	ECLITE_LOG_DEBUG("ucsi_version: %x",
			 eclite_opregion.ucsi_in_data.ucsi_version);

	eclite_send_event(ECLITE_EVENT_UCSI_UPDATE);
}

static void timer_event_process(void)
{
	/* CPU callback.*/
	struct dispatcher_queue_data event_data;

	event_data.event_type = THERMAL_EVENT;
	event_data.data = 0;
	eclite_post_dispatcher_event(&event_data);

}

static void eclite_dispatcher(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct dispatcher_queue_data queue_data;
	uint16_t thermal_status;
	uint16_t charger_status;

	while (1) {
		/**
		 * Wait on queue
		 * 1. decode the event received.
		 * 2. if CHG/FG event Call chargerframework_process_event()
		 * 3. if thermal event call thermalframework_process_event()
		 * 4. if HECI event call heci_process_event(event);
		 */
		k_msgq_get(&dispatcher_queue, &queue_data, K_FOREVER);
		ECLITE_LOG_DEBUG("Queue event_type = %d",
				 queue_data.event_type);
		switch (queue_data.event_type) {
		case HECI_EVENT:
			ECLITE_LOG_DEBUG("Dispatcher HECI Event");
			/* process data recived from OS and save in buffer
			 * queue event based on commands/event from OS
			 */
			eclite_heci_event_process(queue_data.data);
			break;
		case TIMER_EVENT:
			/* callback periodic polling events */
			ECLITE_LOG_DEBUG("Periodic timer Evt Msg");
			timer_event_process();
			break;
		case GPIO_EVENT:
			ECLITE_LOG_DEBUG("Dispatcher GPIO Event");
			/* process gpio isr and queue event based on gpio */
			gpio_event_processing(platform_devices,
					      queue_data.data);
			break;
		case CHG_EVENT:
		case FG_EVENT:
			if (thermal_disable_d0ix) {
				break;
			}
			charger_status = 0;
			ECLITE_LOG_DEBUG("Periodic battery Evt Msg");
			/* Command Processing */
			if (queue_data.data) {
				process_charger_command(queue_data.data);
				break;
			}

			/* Callback processing */
			charging_manager_callback(queue_data.event_type,
						  &charger_status);
			update_opregion(DEV_CHG);
			eclite_send_charger_event(&charger_status);
			if (charger_status & BIT2) {
				eclite_send_event(ECLITE_EVENT_BATTERY);
			}
			handle_low_battery_wake(&eclite_opregion);
			handle_crit_battery_shutdown(&eclite_opregion);
			break;
		case THERMAL_EVENT:
			if (thermal_disable_d0ix) {
				break;
			}
			ECLITE_LOG_DEBUG("Periodic thermal Evt Msg");
			thermal_status = 0;
			/* Command Processing */
			if (queue_data.data) {
				process_thermal_command(queue_data.data);
				break;
			}
			/* Callback processing */
			thermal_callback(&thermal_status);
			update_opregion(DEV_THERMAL);
			update_opregion(DEV_FAN);
			update_opregion(DEV_THERMAL_CPU);
			eclite_send_thermal_event(&thermal_status);
			handle_crit_thermal_shutdown(&eclite_opregion);
			break;
		case UCSI_EVENT:
			ECLITE_LOG_DEBUG("Periodic UCSI Event Msg");
			/* Command Processing */
			if (queue_data.data) {
				process_ucsi_command(queue_data.data);
			} else {
				eclite_send_ucsi_event();
			}
			break;
		default:
			ECLITE_LOG_DEBUG("Unknown Event ID %d",
					 queue_data.event_type);
			break;
		}
	}
}

int eclite_post_dispatcher_event(struct dispatcher_queue_data *event_data)
{
	int ret;

	ret = k_msgq_put(&dispatcher_queue, event_data, K_NO_WAIT);
	return ret;
}

static int gpio_event_processing(struct eclite_device
				 *platform_gpio[], uint32_t gpio_number)
{
	struct dispatcher_queue_data event_data;

	for (int i = 0; i < no_of_devices; i++) {
		void *gpio_dev = platform_gpio[i]->hw_interface->gpio_dev;
		struct platform_gpio_config *gpio_cfg =
			(struct platform_gpio_config *)
			platform_gpio[i]->hw_interface->gpio_config;

		if (gpio_number == gpio_cfg->gpio_no) {
			ECLITE_LOG_DEBUG("gpio_number:%d", gpio_number);
			if (platform_gpio[i]->isr != NULL) {
				if (platform_gpio[i]->isr((void *)
							  platform_gpio[i])) {
					ECLITE_LOG_DEBUG("error in ISR");
				}
			}
			/* queue event corresponding to gpio */
			switch (platform_gpio[i]->device_typ) {
			case DEV_CHG:
				event_data.event_type = CHG_EVENT;
				break;
			case DEV_FG:
				event_data.event_type = FG_EVENT;
				break;
			case DEV_THERMAL:
				event_data.event_type = THERMAL_EVENT;
				break;
			case DEV_FAN:   /* to check event type*/
				event_data.event_type = THERMAL_EVENT;
				break;
			case DEV_UCSI:
				event_data.event_type = UCSI_EVENT;
				break;
			default:        /* unknown device type */
				ECLITE_LOG_DEBUG("UNKNOWN EVENT");
				break;
			}
			event_data.data = 0;
			eclite_post_dispatcher_event(&event_data);
		} else {
			continue;
		}
		if (gpio_dev) {
			ECLITE_LOG_DEBUG("Interrupt for %s serviced",
					 platform_gpio[i]->name);
			/* re-enable interrupt callback for device */
			uint32_t gpio_pin_flag = gpio_cfg->gpio_config.dir |
						 gpio_cfg->gpio_config.pull_down_en |
						 gpio_cfg->gpio_config.intr_type;

			eclite_gpio_pin_enable_callback(gpio_dev, gpio_number,
							gpio_pin_flag);
		}
	}
	return 0;
}

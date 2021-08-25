/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#ifdef CONFIG_TEST_MODE
#include <sys/sys_io.h>
#endif
#include "platform.h"
#include "common.h"
#include "eclite_device.h"
#include "eclite_service.h"
#include "eclite_hostcomm.h"
#include "eclite_dispatcher.h"

#include "charger_framework.h"
#include "thermal_framework.h"
#include "eclite_hw_interface.h"
#include "eclite_opregion.h"
#include <sedi.h>

LOG_MODULE_REGISTER(eclite, CONFIG_ECLITE_LOG_LEVEL);

/* Platform config */
#ifdef CONFIG_ECLITE_CHARGING_FRAMEWORK
#include "bq40z40.h"
#include "bq24610.h"
#endif /*  CONFIG_ECLITE_CHARGING_FRAMEWORK */

#ifdef CONFIG_THERMAL_ENABLE
#include "tmp102.h"
#include <fan.h>
#include "cpu.h"
#endif /* CONFIG_THERMAL_ENABLE */
#ifdef CONFIG_ECLITE_UCSI_FRAMEWORK
#include "ccg.h"
#endif

/*unit is mili sec*/
#define ECLITE_BOOT_WAIT_TIME   4000

K_THREAD_STACK_EXTERN(dispatcher_stack);
K_TIMER_DEFINE(dispatcher_timer, timer_periodic_callback, timer_stop_callback);

/*Platform devices */
APP_GLOBAL_VAR(1) struct eclite_device *platform_devices_default[] = {
#ifdef CONFIG_ECLITE_CHARGING_FRAMEWORK
	&bq40z40_device_fg,
	&bq24610_device_charger,
#endif  /* CONFIG_ECLITE_CHARGING_FRAMEWORK */

#ifdef CONFIG_THERMAL_ENABLE
	&tmp102_sensor1,
	&tmp102_sensor2,
	&tmp102_sensor3,
	&tmp102_sensor4,
	&thermal_cpu,
	&fan_device,
#endif  /* CONFIG_THERMAL_ENABLE */
#ifdef CONFIG_ECLITE_UCSI_FRAMEWORK
	&ccg_pd_device
#endif  /* CONFIG_ECLITE_UCSI_FRAMEWORK */
};

APP_GLOBAL_VAR(1) int eclite_fan_speed;
/*Platform devices without CPU thermal*/
APP_GLOBAL_VAR(1) struct eclite_device
*platform_devices_without_cpu_thermal[] = {
#ifdef CONFIG_ECLITE_CHARGING_FRAMEWORK
	&bq40z40_device_fg,
	&bq24610_device_charger,
#endif  /* CONFIG_ECLITE_CHARGING_FRAMEWORK */

#ifdef CONFIG_THERMAL_ENABLE
	&tmp102_sensor1,
	&tmp102_sensor2,
	&tmp102_sensor3,
	&tmp102_sensor4,
	&fan_device,
#endif  /* CONFIG_THERMAL_ENABLE */
#ifdef CONFIG_ECLITE_UCSI_FRAMEWORK
	&ccg_pd_device
#endif  /* CONFIG_THERMAL_ENABLE */
};

/*Handler for platform devices */
APP_GLOBAL_VAR(1) struct eclite_device **platform_devices
	= platform_devices_default;

#ifdef CONFIG_ECLITE_CHARGING_FRAMEWORK
static APP_GLOBAL_VAR(1) struct  platform_gpio_config battery_gpio_config = {
	.gpio_no = BATTERY_GPIO,
	.gpio_config = {
		.pin_bit_mask = 1ULL << BATTERY_GPIO,
		.dir = GPIO_INPUT,
		.pull_down_en = 0UL,
		.intr_type = GPIO_INT_EDGE_RISING | GPIO_INT_DEBOUNCE,
	},
};

static APP_GLOBAL_VAR(1) struct  platform_gpio_config charger_gpio_config = {
	.gpio_no = CHARGER_GPIO,
	.gpio_config = {
		.pin_bit_mask = 1ULL << CHARGER_GPIO,
		.dir = GPIO_INPUT,
		.pull_down_en = 0UL,
		.intr_type = GPIO_INT_EDGE_BOTH | GPIO_INT_DEBOUNCE,
	},
};

#endif /* CONFIG_ECLITE_CHARGING_FRAMEWORK */

#ifdef CONFIG_THERMAL_ENABLE

static APP_GLOBAL_VAR(1) struct  platform_gpio_config thermal_1_gpio_config = {
	.gpio_no = THERMAL_SENSOR_0_GPIO,
	.gpio_config = {
		.pin_bit_mask = 1ULL << THERMAL_SENSOR_0_GPIO,
		.dir = GPIO_INPUT,
		.pull_down_en = 0UL,
		.intr_type = GPIO_INT_EDGE_FALLING | GPIO_INT_DEBOUNCE,
	},
};

static APP_GLOBAL_VAR(1) struct  platform_gpio_config thermal_2_gpio_config = {
	.gpio_no = THERMAL_SENSOR_1_GPIO,
	.gpio_config = {
		.pin_bit_mask = 1ULL << THERMAL_SENSOR_1_GPIO,
		.dir = GPIO_INPUT,
		.pull_down_en = 0UL,
		.intr_type = GPIO_INT_EDGE_FALLING | GPIO_INT_DEBOUNCE,
	},
};

static APP_GLOBAL_VAR(1) struct  platform_gpio_config thermal_3_gpio_config = {
	.gpio_no = THERMAL_SENSOR_2_GPIO,
	.gpio_config = {
		.pin_bit_mask = 1ULL << THERMAL_SENSOR_2_GPIO,
		.dir = GPIO_INPUT,
		.pull_down_en = 0UL,
		.intr_type = GPIO_INT_EDGE_FALLING | GPIO_INT_DEBOUNCE,
	},
};

static APP_GLOBAL_VAR(1) struct  platform_gpio_config thermal_4_gpio_config = {
	.gpio_no = THERMAL_SENSOR_3_GPIO,
	.gpio_config = {
		.pin_bit_mask = 1ULL << THERMAL_SENSOR_3_GPIO,
		.dir = GPIO_INPUT,
		.pull_down_en = 0UL,
		.intr_type = GPIO_INT_EDGE_FALLING | GPIO_INT_DEBOUNCE,
	},
};

#endif /* CONFIG_THERMAL_ENABLE */

#ifdef CONFIG_ECLITE_UCSI_FRAMEWORK
static APP_GLOBAL_VAR(1) struct  platform_gpio_config ucsi_gpio_config = {
	.gpio_no = UCSI_GPIO,
	.gpio_config = {
		.pin_bit_mask = 1ULL << UCSI_GPIO,
		.dir = GPIO_INPUT,
		.pull_down_en = 0UL,
		.intr_type = GPIO_INT_EDGE_FALLING | GPIO_INT_DEBOUNCE,
	},
};
#endif
APP_GLOBAL_VAR(1) struct platform_gpio_list plt_gpio_list[] = {
#ifdef CONFIG_ECLITE_CHARGING_FRAMEWORK
	{ BATTERY_GPIO,    eclite_service_isr, 0 },
	{ CHARGER_GPIO,    eclite_service_isr, 0 },
#endif  /*  CONFIG_ECLITE_CHARGING_FRAMEWORK */
#ifdef CONFIG_THERMAL_ENABLE
	{ THERMAL_SENSOR_0_GPIO, eclite_service_isr, 0 },
	{ THERMAL_SENSOR_1_GPIO, eclite_service_isr, 0 },
	{ THERMAL_SENSOR_2_GPIO, eclite_service_isr, 0 },
	{ THERMAL_SENSOR_3_GPIO, eclite_service_isr, 0 },
#endif  /* CONFIG_THERMAL_ENABLE */
#ifdef CONFIG_ECLITE_UCSI_FRAMEWORK
	{ UCSI_GPIO,            eclite_service_isr, 0 },
#endif  /* CONFIG_THERMAL_ENABLE */
};

APP_GLOBAL_VAR(1) bool cpu_thermal_enable = true;
APP_GLOBAL_VAR(1) bool eclite_enable = true;
APP_GLOBAL_VAR(1) int eclite_sx_state = PM_RESET_TYPE_S0;
APP_GLOBAL_VAR(1) uint32_t no_plt_gpio = sizeof(plt_gpio_list) /
					 sizeof(struct platform_gpio_list);

APP_GLOBAL_VAR(1) uint32_t no_of_devices = sizeof(platform_devices_default) /
					   sizeof(struct eclite_device *);

APP_GLOBAL_VAR(1) uint8_t thermal_disable_d0ix = 0;

static void connect_peripherals(void);

#if defined(CONFIG_SOC_INTEL_PSE)
/* Add the requried kernel object pointer into list. */

static const void *obj_list[] = {
	&dispatcher_task, &dispatcher_stack, &dispatcher_queue,
	&dispatcher_timer,
};

#define NO_KOBJS 4

static void service_main(void *p1, void *p2, void *p3);

/**
 * Define a service which will execute immediately
 * after post kernel init but before Apps main.
 *
 * ec_lite_fw is configured to run as with service id 1.
 *
 * Note: The service main thread will be invoked in coopertive mode
 * and service main entry will invoke even before Zephyr main function.
 * Service main should complete as quickly as possible and offload
 * all its time consuming work to a sub task to improve boot latency.
 * The fuction connect_peripherals is chosen to be the callback funtion
 * for this service, and all the binding/callback configurations should
 * be performed in this function.
 * We cannot compute the amount of Kernel objects because each element
 * has different sizes, this is why everytime we add a new kernel object,
 * we must manually change the value in macro NO_KOBJS.
 **/


DEFINE_USER_MODE_APP(1, USER_MODE_SERVICE | K_PART_GLOBAL | K_PART_SHARED,
		     (k_thread_entry_t)service_main, 1024, (void **)obj_list,
		     NO_KOBJS, connect_peripherals);
#endif

void sx_callback(sedi_pm_sx_event_t event, void *ctx)
{
	int ret;
	sedi_pm_reset_prep_t sx_state;

	ret = sedi_pm_get_reset_prep_info(&sx_state);
	if (ret == SEDI_DRIVER_OK) {
		eclite_sx_state = sx_state.reset_type;
	}
}

void s0ix_callback(sedi_pm_s0ix_event_t event, void *ctx)
{
	struct dispatcher_queue_data event_data;

	if (event == PM_EVENT_HOST_S0IX_ENTRY) {
		k_timer_stop(&dispatcher_timer);

		eclite_fan_speed = eclite_opregion.pwm_dutycyle;
		/* Turn off the FAN */
		eclite_opregion.pwm_dutycyle = 0;
		event_data.event_type = THERMAL_EVENT;
		event_data.data = ECLITE_OS_EVENT_PWM_UPDATE;
		eclite_post_dispatcher_event(&event_data);

	} else {
		/* Turn on the FAN */
		eclite_opregion.pwm_dutycyle = eclite_fan_speed;
		event_data.event_type = THERMAL_EVENT;
		event_data.data = ECLITE_OS_EVENT_PWM_UPDATE;
		eclite_post_dispatcher_event(&event_data);

		k_timer_start(&dispatcher_timer, K_NO_WAIT,
			      K_MSEC(CONFIG_ECLITE_POLLING_TIMER_PERIOD));
	}
}

int platform_gpio_register_wakeup(int battery_state)
{
	sedi_wake_event_instance_t pin;

	if (battery_state == SUCCESS) {
		/* Enable wake source for BTP if battery present */
		pin.gpio_pin = BATTERY_GPIO;
		sedi_pm_configure_wake_source(PM_WAKE_EVENT_GPIO, pin, true);
	}

	/* Enable wake source for Platform thermal sensors */
	pin.gpio_pin = THERMAL_WAKE_SOURCE_PIN;
	sedi_pm_configure_wake_source(PM_WAKE_EVENT_GPIO, pin, true);

	return 0;
}

static void connect_peripherals(void)
{
	/* This function needs the information provided by
	 * init_dev_framwork, so this why it is placed here.
	 */
	ECLITE_LOG_DEBUG(" ");
	int ret;
	int battery_state = FAILURE;
#ifdef CONFIG_TEST_MODE
	int cnt = 0;
#endif
	if (sedi_get_config(SEDI_CONFIG_ECLITE_EN, NULL) != SEDI_CONFIG_SET) {
		LOG_ERR("ECLite service: Disabled");
		eclite_enable = false;
		return;
	}

	if (sedi_get_config(SEDI_CONFIG_ECLITE_DTS_EN,
			    NULL) != SEDI_CONFIG_SET) {
		cpu_thermal_enable = false;
		platform_devices = platform_devices_without_cpu_thermal;
		no_of_devices = sizeof(platform_devices_without_cpu_thermal) /
				sizeof(struct eclite_device *);
	}
	init_dev_framework(platform_devices, no_of_devices);
#ifdef CONFIG_TEST_MODE
	/* ECLite FW boot should wait till linux boots. This 4sec time is
	 * derived with experiments
	 */
	k_sleep(K_MSEC(ECLITE_BOOT_WAIT_TIME));
	/* This is for the mux settings. Mus setting need to be done using
	 * simics command until RVP is arrived.
	 */
	while (!sys_read32(0x40400b00)) {
		k_sleep(K_MSEC(100));
		ECLITE_LOG_DEBUG("%3d %d\r", cnt++, sys_read32(0x40400b00));
	}
#endif

#ifdef CONFIG_ECLITE_CHARGING_FRAMEWORK
	struct eclite_device *charger_dev = find_dev_by_type(DEV_CHG);

	/* Look for charger device, if not found dont do intialization */
	if (charger_dev) {
		struct charger_driver_data *charger_data =
			charger_dev->driver_data;
		/* configure peripharals connected to charger */
		charger_dev->hw_interface->gpio_config =
			(void *)&charger_gpio_config;
		charger_data->i2c_slave_addr = CHARGER_I2C_SLAVE_ADDRESS;
		charger_dev->init(charger_dev);
	}

	struct eclite_device *battery_dev = find_dev_by_type(DEV_FG);

	/* Look for battery device, if not found dont do intialization */
	if (battery_dev) {
		struct sbs_driver_data *battery_data =
			battery_dev->driver_data;
		/* configure peripharals connected to battery */
		battery_dev->hw_interface->gpio_config =
			(void *)&battery_gpio_config;
		battery_data->sbs_i2c_slave_addr = BATTERY_I2C_SLAVE_ADDRESS;
		battery_state = battery_dev->init(battery_dev);
	}

#endif  /* CONFIG_ECLITE_CHARGING_FRAMEWORK */
#ifdef CONFIG_THERMAL_ENABLE
	struct eclite_device *fan = find_dev_by_type(DEV_FAN);

	/* Look for fan device, if not found dont do intialization */
	if (fan) {
		/* configure peripherals connected to fan.*/
		fan->init(fan);
	}
	struct eclite_device *thermal;
	struct thermal_driver_data *thermal_data;
#endif  /* CONFIG_THERMAL_ENABLE */


#ifdef CONFIG_THERMAL_ENABLE
	/* configure peripharals connected to thermals */
	thermal = find_dev_instance_by_type(DEV_THERMAL, INSTANCE_1);
	if (thermal) {
		thermal->hw_interface->gpio_config = &thermal_1_gpio_config;
		thermal_data = thermal->driver_data;
		thermal_data->i2c_slave = THERMAL_SENSOR_0_I2C;
		thermal->init(thermal);
	}

	thermal = find_dev_instance_by_type(DEV_THERMAL, INSTANCE_2);
	if (thermal) {
		thermal->hw_interface->gpio_config = &thermal_2_gpio_config;
		thermal_data = thermal->driver_data;
		thermal_data->i2c_slave = THERMAL_SENSOR_1_I2C;
		thermal->init(thermal);
	}

	thermal = find_dev_instance_by_type(DEV_THERMAL, INSTANCE_3);
	if (thermal) {
		thermal->hw_interface->gpio_config = &thermal_3_gpio_config;
		thermal_data = thermal->driver_data;
		thermal_data->i2c_slave = THERMAL_SENSOR_2_I2C;
		thermal->init(thermal);
	}

	thermal = find_dev_instance_by_type(DEV_THERMAL, INSTANCE_4);
	if (thermal) {
		thermal->hw_interface->gpio_config = &thermal_4_gpio_config;
		thermal_data = thermal->driver_data;
		thermal_data->i2c_slave = THERMAL_SENSOR_3_I2C;
		thermal->init(thermal);
	}

	/* Initialize opregion parameters for default thermal data */
	initialize_opregion(DEV_THERMAL);
	initialize_opregion(DEV_THERMAL_CPU);
#endif  /* CONFIG_THERMAL_ENABLE */

#ifdef CONFIG_ECLITE_UCSI_FRAMEWORK
	struct eclite_device *dev = find_dev_by_type(DEV_UCSI);

	if (dev) {
		/* configure peripharals connected to ucsi */
		dev->hw_interface->gpio_config = (void *)&ucsi_gpio_config;
		dev->init(dev);
		initialize_opregion(DEV_UCSI);
		ECLITE_LOG_DEBUG("UCSI init finished");
	}
#endif
	ret = eclite_service_gpio_config((void *)platform_devices,
					 (void *)plt_gpio_list,
					 no_of_devices,
					 no_plt_gpio);
	/* event notification is initialized in Eclite Fw. Any event generated
	 * before BIOS boot will be blocked by Eclite driver.
	 */
	eclite_opregion.event_notify_config = 1;
	if (ret) {
		ECLITE_LOG_DEBUG("Failed to configure GPIOs: %d", ret);
	}

	if (cpu_thermal_enable == true) {
		/* Create polling timer */
		k_timer_init(&dispatcher_timer, timer_periodic_callback,
			     timer_stop_callback);

		LOG_DBG("Registering for S0ix");
		sedi_pm_register_s0ix_notification(s0ix_callback, NULL);
	}

	sedi_pm_register_sx_notification(sx_callback, NULL);
	platform_gpio_register_wakeup(battery_state);
}

void eclite_d0ix(uint8_t state)
{
	thermal_disable_d0ix = state;
}
#if defined(CONFIG_SOC_INTEL_PSE)
static void service_main(void *p1, void *p2, void *p3)
{
	if (!eclite_enable) {
		LOG_ERR("ECLite service: Disabled");
		return;
	}

#ifdef CONFIG_THERMAL_ENABLE
	if (cpu_thermal_enable) {
		struct eclite_device *thermal;

		thermal = find_dev_by_type(DEV_THERMAL_CPU);
		if (thermal) {
			thermal->init(thermal);
		}
	}
#endif  /* CONFIG_THERMAL_ENABLE */
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	dispatcher_init();
}
#else
void main(void)
{
	dispatcher_init();
	connect_peripherals();
}
#endif

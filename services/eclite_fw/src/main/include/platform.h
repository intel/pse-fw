/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief This file exposes interface for platform initialization.
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <drivers/gpio.h>
#include <errno.h>
#include <logging/log.h>
#include <time.h>

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are board configuration macros.
 */

#if defined(CONFIG_SOC_ELKHART_LAKE_PSE)
#include <kernel_structs.h>
#include <user_app_framework/user_app_framework.h>
#endif /* CONFIG_SOC_ELKHART_LAKE_PSE */

#define TEMP_CPU_CRIT_SHUTDOWN CONFIG_CPU_CRIT_SHUTDOWN
#define TEMP_SYS_CRIT_SHUTDOWN CONFIG_SYS_CRIT_SHUTDOWN
#define POWRUP_DEFAULT_FAN_SPEED 50

/** Macro defines maximum number of sensor thermal framework supports.*/
#define MAX_THERMAL_SENSOR 4

/* Configuration for atmel board */
#ifdef _SAM_E70_XPLAINED_

#define BATTERY_GPIO  9
#define BATTERY_I2C_SLAVE_ADDRESS 0xB

#define CHARGER_GPIO  10
#define CHARGER_CONTROL 11
#define CHARGER_I2C_SLAVE_ADDRESS 0xB

#define ECLITE_I2C_DEV  I2C_0_LABEL
#define ECLITE_GPIO_DEV DT_GPIO_SAM_PORTA_LABEL
#define ECLITE_PWM_DEV "PWM_0"
#define ECLITE_TACHO_DEV "TGPIO_0"

#define THERMAL_SENSOR_0_GPIO                   6
#define THERMAL_SENSOR_0_I2C                    0x48

#define THERMAL_SENSOR_1_GPIO                   7
#define THERMAL_SENSOR_1_I2C                    0xC

#define THERMAL_SENSOR_2_GPIO                   8
#define THERMAL_SENSOR_2_I2C                    0xC

#define THERMAL_SENSOR_3_GPIO                   12
#define THERMAL_SENSOR_3_I2C                    0xC

#endif /* _SAM_E70_XPLAINED_ */



#define ECLITE_I2C_DEV                  CONFIG_ECLITE_I2C_SLAVE_NAME

#define PWM_PIN                         CONFIG_ECLITE_FAN_PWM_PIN
#define ECLITE_PWM_DEV                  CONFIG_ECLITE_FAN_PWM_NAME
#define TGPIO_PIN                       CONFIG_ECLITE_FAN_TGPIO_PIN
#define ECLITE_TACHO_DEV                CONFIG_ECLITE_FAN_TGPIO_NAME

#define BATTERY_GPIO_NAME       CONFIG_ECLITE_BATTERY_BTP_GPIO_NAME
#define BATTERY_GPIO            CONFIG_ECLITE_BATTERY_BTP_GPIO_PIN
#define BATTERY_I2C_SLAVE_ADDRESS 0xB

#define ECLITE_UCSI_I2C_DEV             CONFIG_ECLITE_UCSI_I2C_SLAVE_NAME
#define UCSI_GPIO_NAME                  CONFIG_ECLITE_UCSI_GPIO_NAME
#define UCSI_GPIO                               CONFIG_ECLITE_UCSI_GPIO_PIN
#define UCSI_I2C_SLAVE_ADDRESS  CONFIG_ECLITE_UCSI_I2C_SLAVE_ADDR

#define CHARGER_GPIO_NAME               CONFIG_ECLITE_CHG_GPIO_NAME
#define CHARGER_GPIO                    CONFIG_ECLITE_CHG_GPIO_PIN
#define CHARGER_CONTROL_NAME            CONFIG_ECLITE_CHG_CTRL_GPIO_NAME
#define CHARGER_CONTROL                 CONFIG_ECLITE_CHG_CTRL_GPIO_PIN
#define CHARGER_I2C_SLAVE_ADDRESS       CONFIG_CHG_I2C_SLAVE_ADDR

#define THERMAL_SENSOR_GPIO             CONFIG_ECLITE_THERMAL_SENSOR_GPIO_PIN
#define THERMAL_SENSOR_GPIO_NAME        CONFIG_ECLITE_THERMAL_SENSOR_GPIO_NAME
#define THERMAL_WAKE_SOURCE_PIN          CONFIG_ECLITE_THERMAL_WAKE_SOURCE_PIN

#define THERMAL_SENSOR_0_GPIO           THERMAL_SENSOR_GPIO
#define THERMAL_SENSOR_0_I2C            CONFIG_THERM_SEN_0_I2C_SLAVE_ADDR

#define THERMAL_SENSOR_1_GPIO           THERMAL_SENSOR_GPIO
#define THERMAL_SENSOR_1_I2C            CONFIG_THERM_SEN_1_I2C_SLAVE_ADDR

#define THERMAL_SENSOR_2_GPIO           THERMAL_SENSOR_GPIO
#define THERMAL_SENSOR_2_I2C            CONFIG_THERM_SEN_2_I2C_SLAVE_ADDR

#define THERMAL_SENSOR_3_GPIO           THERMAL_SENSOR_GPIO
#define THERMAL_SENSOR_3_I2C            CONFIG_THERM_SEN_3_I2C_SLAVE_ADDR


int platform_gpio_register_interrupt(void);

/**
 * @endcond
 */


/** @brief Register platform GPIO's with bsp.
 *
 *  @param battery state
 *  @return error codes.
 */
int platform_gpio_register_wakeup(int battery_state);

/** @brief GPIO configuration per GPIO.
 *
 *  Structure declares GPIO configuration for interrupt.
 *************@{
 */
struct platform_gpio_config {
	/** Platform GPIO number.*/
	uint32_t gpio_no;

	/** @brief GPIO configuration.
	 *
	 * @{ GPIO configuration.
	 */
	struct gpio_config_t {
		/** GPIO bit position.*/
		uint32_t pin_bit_mask;
		/** GPIO direction.*/
		uint32_t dir;
		/** GPIO pull up configuration.*/
		uint8_t pull_down_en;
		/** GPIO interrupt configuration.*/
		uint32_t intr_type;
	} gpio_config;
	/**@}*/
	/** GPIO callback.*/
	struct gpio_callback eclite_service_isr_data;
};
/**@}*/


/** @brief GPIO structure is used to register callback
 *
 *  GPIO callback for interrupt.
 */
struct platform_gpio_list {
	/** Platform GPIO number.*/
	uint32_t gpio_no;

	/** interrupt callback function.*/
	void *gpio_config_cb;

	/** data to be sent to queue.*/
	uint8_t is_interrupted;
};

/**
 * @brief  This function does application initialization.
 *
 */
void platform_init(void);

/**
 * @brief EClite device table.
 */
extern struct eclite_device **platform_devices;

/**
 * @brief EClite device table size
 */
extern uint32_t no_of_devices;

/**
 * @brief EClite Thermal Disable flag for D0ix entry
 */
extern uint8_t thermal_disable_d0ix;


#endif /*_PLATFORM_H_ */

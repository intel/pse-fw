/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Peripherals Library.
 */

#ifndef _ECLITE_HW_INTERFACE__H_
#define _ECLITE_HW_INTERFACE__H_

#include <stdint.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <zephyr.h>
#include <device.h>
#include <sys/util.h>
#include <logging/log.h>
#include <drivers/pwm.h>

/* I2C APIs */
/**
 * @brief  This function configures I2C device.
 *
 * @param  dev is i2c device pointer.
 * @param  cfg is i2c device configuration.
 *
 * @retval I2C device error code.
 */

int eclite_i2c_config(void *dev, uint32_t cfg);

/**
 * @brief  This function reads I2C byte from slave device.
 *
 * @param  ptr is I2C device pointer.
 * @param  dev_addr is slave address of device.
 * @param  reg_addr is command need to be executed by device.
 * @param  value is pointer that hold value sent back from device.
 *
 * @retval I2C device error code.
 */
int eclite_i2c_read_byte(void *ptr, uint16_t dev_addr, uint8_t reg_addr,
			 uint8_t *value);

/**
 * @brief  This function writes I2C byte to slave device.
 *
 * @param  ptr is I2C device pointer.
 * @param  dev_addr is slave address of device.
 * @param  reg_addr is command need to be executed by device.
 * @param  value is data to be sent to device.
 *
 * @retval I2C device error code.
 */
int eclite_i2c_write_byte(void *ptr, uint16_t dev_addr, uint8_t reg_addr,
			  uint8_t value);

/**
 * @brief  This function reads I2C word from slave device.
 *
 * @param  ptr is I2C device pointer.
 * @param  dev_addr is slave address of device.
 * @param  reg_addr is command need to be executed by device.
 * @param  value is pointer that hold value sent back from device.
 *
 * @retval I2C device error code.
 */
int eclite_i2c_read_word(void *ptr, uint16_t dev_addr, uint8_t reg_addr,
			 uint16_t *value);

/**
 * @brief  This function writes I2C word to slave device.
 *
 * @param  ptr is I2C device pointer.
 * @param  dev_addr is slave address of device.
 * @param  reg_addr is command need to be executed by device.
 * @param  value is data need to sent to device.
 *
 * @retval I2C device error code.
 */
int eclite_i2c_write_word(void *ptr, uint16_t dev_addr, uint8_t reg_addr,
			  uint16_t value);

int eclite_i2c_burst_write(void *ptr, uint16_t dev_addr, uint8_t reg_addr,
			   uint8_t *value, uint32_t len);

int eclite_i2c_burst_read(void *ptr, uint16_t dev_addr, uint8_t reg_addr,
			  uint8_t *value, uint32_t len);


int eclite_i2c_burst_write16(void *ptr, uint16_t dev_addr, uint16_t reg_addr,
			     uint8_t *value, uint32_t len);

int eclite_i2c_burst_read16(void *ptr, uint16_t dev_addr, uint16_t reg_addr,
			    uint8_t *value, uint32_t len);

/**
 * @brief  This function retrieves I2C device pointer to access slave device.
 *
 * @param  name is I2C device driver name.
 *
 * @retval device pointer.
 */
struct device *eclite_get_i2c_device(const char *name);

/* GPIO APIs */
/**
 * @brief  This function retrieves GPIO device pointer to access slave device.
 *
 * @param  name is GPIO device driver name.
 *
 * @retval device pointer.
 */
struct device *eclite_get_gpio_device(const char *name);

/**
 * @brief  This function drives GPIO pin HIGH/LOW if configure in native mode.
 *
 * @param  dev is GPIO device pointer.
 * @param  pin is GPIO pin index.
 * @param  value is GPIO pin status.
 *
 * @retval GPIO device error code.
 */
int eclite_set_gpio(void *dev, uint32_t pin, uint32_t value);

/**
 * @brief  This function reads GPIO.
 *
 * @param  dev is GPIO device pointer.
 * @param  pin is GPIO pin index.
 * @param  value is pointer which hold GPIO pin status to be programmed.
 *
 * @retval GPIO device error code.
 */
int eclite_get_gpio(void *dev, uint32_t pin, uint32_t *value);

/**
 * @brief  This function enables callback for interrupt.
 *
 * @param  port is GPIO device pointer.
 * @param  pin is GPIO pin index.
 *
 * @retval GPIO device error code.
 */
int eclite_gpio_pin_enable_callback(void *port, uint32_t pin, uint32_t flags);

/**
 * @brief  This function disabled function callback for interrupt.
 *
 * @param  port is GPIO device pointer.
 * @param  pin is GPIO pin index.
 *
 * @retval GPIO device error code.
 */
int eclite_gpio_pin_disable_callback(void *port, uint32_t pin);

/**
 * @brief  This function is wrapper function. It is used to configure GPIO.
 *
 * @param  dev is GPIO device pointer.
 * @param  pin is GPIO pin index.
 * @param  flags is GPIO configuration  detail.
 *
 * @retval GPIO device error code.
 */
int eclite_gpio_configure(void *dev, uint32_t pin, uint32_t flags);

/**
 * @brief  This is wrapper function. It registers GPIO callback.
 *
 * @param dev is GPIO device pointer.
 * @param cb is callback pointer to handle function callback related parameters.
 * @param handler is function callback on interrupt reception.
 * @param pin is GPIO index.
 *
 * @retval GPIO device error code.
 */
int eclite_gpio_register_callback(void *dev, void *cb,
				  gpio_callback_handler_t handler,
				  uint32_t pin);

/* PWM APIs */
/**
 * @brief  This function retrieves PWM device pointer to access slave device.
 *
 * @param  name is PWM device driver name.
 *
 * @retval device pointer.
 */

struct device *eclite_get_pwm_device(const char *name);


/* TACHO APIs */
/**
 * @brief  This function retrieves TACHO device pointer to access slave device.
 *
 * @param  name is TACHO device driver name.
 *
 * @retval device pointer.
 */

struct device *eclite_get_tacho_device(const char *name);

/**
 * @brief  This is wrapper function. It Initializes PWM module.
 *
 * @param dev is PWM device pointer.
 * @param cfg is configuration for PWM module.
 *
 * @retval PWM device error code.
 */
int eclite_init_pwm(void *dev, void *cfg);

/**
 * @brief  This generated PWM pulses.
 *
 * @param dev is PWM device pointer.
 * @param pwm is PWM output pin.
 * @param period is pulse width (HIGH + LOW).
 * @param pulse is High duration width.
 *
 * @retval PWM device error code.
 */
int eclite_write_pwm(void *dev, uint32_t pwm, uint32_t period, uint32_t pulse);

/**
 * @brief  This is wrapper function. It Initializes TACHO module.
 *
 * @param dev is TACHO device pointer.
 * @param cfg is configuration for TACHO module.
 *
 * @retval TACHO device error code.
 */
int eclite_init_tacho(void *dev, void *cfg);

/**
 * @brief  This reads FAN speed.
 *
 * @param dev is TACHO device pointer.
 * @param tacho is fan rotation. This signal is generated by fan.
 *
 * @retval TACHO device error code.
 */
int eclite_read_tacho(void *tgpio_dev, uint32_t *tacho);

/**
 * @brief  This manages commands to be sent to PMC.
 *
 * @param msg_type is enum eclite_msg_type.
 * @param buf is data to be sent/received.
 *
 * @retval PMC error code.
 */
int pmc_command(uint8_t msg_type, uint8_t *buf);

/**shut down.request*/
#define PMC_SRT_SHUTDOWN        BIT4
/**wake up host*/
#define PMC_SRT_WAKEUP          BIT5
/**Global reset.request*/
#define PMC_SRT_GLOB_RST        BIT1
/**Host partition reset with power cycle.*/
#define PMC_SRT_HPRWPC          BIT2
/**Host partition reset without power cycle.*/
#define PMC_SRT_HPRWOPC         BIT3

/** @brief eclite msg type to be sent to PMC
 *
 *  Enum "eclite_msg_type" message name to be sent to/received from PMC.
 */
enum eclite_msg_type {
	GET_TEMP = 1,
	GET_TJ_MAX,
	SEND_POWER_STATE_CHANGE,
};

#endif /* _ECLITE_HW_INTERFACE__H_ */

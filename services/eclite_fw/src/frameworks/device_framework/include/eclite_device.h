/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief provide interface to access eclite device.
 */

#ifndef _ECLITE_DEVICE_H_
#define _ECLITE_DEVICE_H_

#include <stdint.h>
#include <logging/log.h>
#include "common.h"


/** @brief Macro to check if platform device list is NULL.
 *  under "DEVICE_DEBUG" debug print is logged if list is NULL.
 *
 *  @return returns platform error code if device list is NULL.
 */
#define IS_DEV_LIST_NULL(list) ((list == NULL) ? true : false)


/** @brief Macro to check if device pointer list is NULL.
 *
 *  @return returns "true" if device pointer is NULL.
 */
#define IS_DEV_NULL(dev_ptr) ((dev_ptr == NULL) ? true : false)

/** @brief Macro to check if "dev" type is same as "type".
 *
 *  @return true if dev type matches with type in question.
 *          false if dev type does not match with type in question.
 */
#define IS_DEV_TYPE_SAME(dev, type) ((dev == type) ? true : false)

/** @brief Device type for Eclite app.
 *
 *  Enum "device_type" provides a list of devices supported by EClite
 *  application.
 */
enum device_type {
	DEV_CHG,
	DEV_FG,
	DEV_FAN,
	DEV_TACHO,
	DEV_THERMAL,
	DEV_THERMAL_CPU,
	DEV_UCSI,
};

/** @brief number of instance of an eclite device.
 *
 *  Enum "instance_type" provides can be used to show available instances of a
 *  particular device.
 */
enum instance_type {
	INSTANCE_1 = 1,
	INSTANCE_2,
	INSTANCE_3,
	INSTANCE_4,
	INSTANCE_5,
	INSTANCE_6,
	INSTANCE_7,
	INSTANCE_8,
	INSTANCE_9,
};

/** @brief Error codes for Device OPS.
 *
 *  Enum "device_err_code" provides a way to classify the
 *  error that could potentially occur during the various device
 *  operations as part of device power management.
 *
 *  Device driver needs to mark the appropriate error codes
 *  for each device operation request coming in from framework.
 */
enum device_err_code {
	DEV_OPS_SUCCESS = 0,
	DEV_INIT_FAILED = -1,
	DEV_SUSP_FAILED = -2,
	DEV_RESU_FAILED = -3,
	DEV_FORCE_SUSP_FAILED = -4,
	DEV_PWR_OFF_FAILED = -5,
	DEV_OP_NOT_SUPPORTED = -6,
	DEV_INVALID_OP = -7,
	DEV_NOT_FOUND = -8,
	DEV_SHUTDOWN = -9,
};

/** @brief Device Status.
 *
 *  Enum "device_status" provides a way to classify the current
 *  status of device that would change during the various device
 *  operations as part of device power management.
 *
 *  Device driver needs to mark the appropriate device state
 *  during each device operation request coming in from framework.
 */
enum device_status {
	DEV_IDLE,
	DEV_BUSY,
	DEV_SUSPEND,
	DEV_POWERED_OFF,
	DEV_STATUS_UNKNOWN,
};

/**
 * @brief Device interrupt service routine. It is called by platform when there
 * is an interrupt on intr_num. Interpretation of intr_num is up to
 * the device driver. Device driver can map intr_num to appropriate
 * functionality.
 *
 * @param device Pointer to the device structure.
 * @param intr_num Interrupt number to be interpreted by device driver.
 */
typedef void (*device_isr_t)(void *device, uint32_t intr_num);

/** @brief Eclite device HW interface
 *
 *  Structure "hw_configuration" defines device HW interface like I2C, GPIO,
 *  etc.
 */
struct hw_configuration {
	/** pointer to GPIO configuration.*/
	void *gpio_config;
	/** pointer GPIO device driver.*/
	void *gpio_dev;
	/** pointer to handle device specifica data. this can be used by driver
	 *  to manage device
	 */
	void *device;
};

/** @brief Device structure for eclite devices.
 *
 * Structure "eclite_device" defines device structure for EC related devices.
 */
struct eclite_device {
	/** device name.*/
	char *name;
	/** device type.*/
	enum device_type device_typ;
	/** instance is used to identify if there are more than one device of
	 * same device type.
	 */
	enum instance_type instance;
	/** device status at the time of initialization.*/
	enum device_status device_sts;
	/** call back initialize the device.*/
	enum device_err_code (*init)(void *device);
	/** pointer to structure containing the API functions for the device
	 * type. This pointer is filled in by the driver at init time.
	 */
	void *driver_api;
	/** driver instance data. For driver use only.*/
	void *driver_data;
	/** HW interface */
	struct hw_configuration *hw_interface;
	/** Interrupt service routine to be called in case driver supports.
	 * To be defined by the driver, NULL if no interrupt required
	 * by device driver.
	 */
	enum device_err_code (*isr)(void *device);
};

#ifdef CONFIG_DEVICE_DEBUG
/**
 * API to print devices in the list
 * prints device name and type
 * @param  None.
 */
void print_dev_list(void);
#endif /* CONFIG_DEVICE_DEBUG */

/** @brief Callback to initialize device framework.
 *
 *  Called to initialize device management framework.
 *
 *  @param plat_dev_list array of devices in a platform.
 *  @param num_of_devices number of devices in array.
 *  @return device error code.
 */
int init_dev_framework(struct eclite_device *plat_dev_list[],

		       int num_of_devices);

/** @brief Callback to initialize a device.
 *
 *  Called to initialize a device which is part of device
 *  device management framework.
 *
 *  @param type type of device to be matched with.
 *  @return device error code.
 */
enum device_err_code init_device(enum device_type type);

/** @brief Get device status.
 *
 *  Get the device status whose type matches with "type"
 *  which is passed as argument to this function/call.
 *
 *  @param type type of device to be matched with.
 *  @return device status.
 */
enum device_status get_device_status(enum device_type type);

/** @brief Get device pointer.
 *
 *  Get the pointer to device whose type matches with "type"
 *  passed as argument to this function/call.
 *
 *  @param type type of device to be matched with.
 *  @return pointer to device structure.
 */
struct eclite_device *find_dev_by_type(enum device_type type);

/** @brief Get device pointer.
 *
 *  get device instance base on device type and instance.
 *
 *  @param type type of device to be matched with.
 *  @param instance insance of device to be matched with.
 *
 *  @return pointer to device structure.
 */

struct eclite_device *find_dev_instance_by_type(enum device_type type,
						uint8_t instance);

#endif /*_ECLITE_DEVICE_H_ */

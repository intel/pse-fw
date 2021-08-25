/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>

#include "eclite_device.h"
#include "common.h"
#include "platform.h"

LOG_MODULE_REGISTER(eclite_device, CONFIG_ECLITE_LOG_LEVEL);

APP_GLOBAL_VAR_BSS(1) struct eclite_device **dev_list;
APP_GLOBAL_VAR_BSS(1) int no_of_devs;

#ifdef CONFIG_DEVICE_DEBUG
/* Print the list of platform devices. */
void print_dev_list(void)
{
	if (dev_list != NULL) {
		for (int i = 0; i < no_of_devs; i++) {
			ECLITE_LOG_DEBUG("Device name: %s\t device type %d",
					 dev_list[i]->name,
					 dev_list[i]->device_typ);
		}
	}
}
#endif /* CONFIG_DEVICE_DEBUG */

/* Find device struct by device type; else returns NULL. */
struct eclite_device *find_dev_instance_by_type(enum device_type type,
						uint8_t instance)
{
	struct eclite_device *dev = NULL;

	if (dev_list == NULL) {
		LOG_ERR("Device list is null");
		return dev;
	}

	for (int i = 0; i < no_of_devs; i++) {
		if ((IS_DEV_TYPE_SAME(dev_list[i]->device_typ, type))) {
			if (dev_list[i]->instance == instance) {
				return dev_list[i];
			}
		}
	}

	return dev;
}

/* Find device struct by device type; else returns NULL. */
struct eclite_device *find_dev_by_type(enum device_type type)
{
	struct eclite_device *dev = NULL;

	if (dev_list == NULL) {
		LOG_ERR("Device list is null");
		return dev;
	}

	for (int i = 0; i < no_of_devs; i++) {
		if ((IS_DEV_TYPE_SAME(dev_list[i]->device_typ, type))) {
			return dev_list[i];
		}
	}

	return dev;
}
/* Get device status captured in dev structure. */
enum device_status get_device_status(enum device_type type)
{
	enum device_status sts;

	if (IS_DEV_LIST_NULL(dev_list)) {
		LOG_ERR("Device not found");
		return DEV_NOT_FOUND;
	}

	/* Run through the global list. */
	for (int i = 0; i < no_of_devs; i++) {
		if (IS_DEV_TYPE_SAME(dev_list[i]->device_typ, type)) {
			sts = dev_list[i]->device_sts;
			return sts;
		}
	}

	return DEV_STATUS_UNKNOWN;
}

/* Initialize a device. */
enum device_err_code init_device(enum device_type type)
{
	int sts;

	if (IS_DEV_LIST_NULL(dev_list)) {
		LOG_ERR("Device not found");
		return DEV_NOT_FOUND;
	}

	/* Run through the global list. */
	for (int i = 0; i < no_of_devs; i++) {
		if (IS_DEV_TYPE_SAME(dev_list[i]->device_typ, type)) {
			sts =  dev_list[i]->init(dev_list[i]);
			return sts;
		}
	}

	return DEV_NOT_FOUND;
}

/* Initialize device framework by passing platform device list. */
int init_dev_framework(struct eclite_device *platform_dev_list[],
		       int num_of_devices)
{
	if (IS_DEV_LIST_NULL(platform_dev_list)) {
		LOG_ERR("Device not found");
		return DEV_NOT_FOUND;
	}

	/* Assign it to dev size. */
	no_of_devs = num_of_devices;

	/* Copy the pointer of platform device array. */
	dev_list = platform_dev_list;

	ECLITE_LOG_DEBUG("Dev framework init done");

	return 0;
}

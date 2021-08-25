/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ucsi.h>
#include <eclite_device.h>

LOG_MODULE_REGISTER(ucsi, CONFIG_ECLITE_LOG_LEVEL);

/* This framework will be called in PD INT context */
int ucsi_framework(uint8_t cmd_id)
{
	int ret;
	struct eclite_device *dev = find_dev_by_type(DEV_UCSI);

	/* Look for USCI device, if not found return error */
	if (!dev) {
		LOG_ERR("No UCSI device found");
		return ERROR;
	}

	struct ucsi_api_t *api = dev->driver_api;

	/* Look for driver api, if not found return error */
	if (!api) {
		LOG_ERR("No UCSI driver APIs");
		return ERROR;
	}

	ECLITE_LOG_DEBUG(" ");

	/* call commands from the command table	*/
	ret = api->ucsi_fn(dev);

	ARG_UNUSED(cmd_id);

	return ret;
}

/* This framework will be called in HECI / when framework wants
 * to send data context read and write data to and from HECI buffer
 * framework buffer
 */

int ucsi_command(uint8_t cmd, uint8_t len, uint8_t *buf)
{
	int ret = 0;
	struct eclite_device *dev = find_dev_by_type(DEV_UCSI);

	/* Look for USCI device, if not found return error */
	if (!dev) {
		LOG_ERR("No UCSI device found");
		return ERROR;
	}

	struct ucsi_data_t *data = dev->driver_data;

	/* Look for driver data, if not found return error */
	if (!data) {
		LOG_ERR("No UCSI driver data");
		return ERROR;
	}

	ECLITE_LOG_DEBUG(" ");

	ARG_UNUSED(len);

	switch (cmd) {
	case USBC_MSG_IN:
		memcpy(buf, &data->msg_in, sizeof(data->msg_in));
		memcpy(buf + sizeof(data->msg_in), &data->cci,
		       sizeof(data->cci));
		break;
	case USBC_MSG_OUT:
		memcpy(&data->msg_out, buf, sizeof(data->msg_out));
		memcpy(&data->control, buf + sizeof(data->msg_out),
		       sizeof(data->control));
		break;
	default:
		LOG_ERR("Unknown UCSI command");
		ret = ERROR;
		break;
	}

	return ret;
}


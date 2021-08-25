/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bq40z40.h"
#include "eclite_hw_interface.h"
#include "platform.h"
#include "charger_framework.h"
#include "eclite_device.h"
#include <toolchain/gcc.h>

LOG_MODULE_REGISTER(battery, CONFIG_ECLITE_LOG_LEVEL);

/* To check the negative bit position.*/
#define CHECK_NEGATIVE_BIT 0x8000
#define BTP_MFG_ADDR 0x2
#define BTP_MFG_RD_SIZE 0x3
#define BTP_MFG_ADDR_MASK 0xFF
#define BTP_MFG_ADDR_SHIFT 0x8
#define BTP_MFG_WR_SIZE 0x3
#define BTP_EN_BUF_SIZE 0x4
#define BTP_EN_RD_SIZE 0x1
#define BTP_EN_KEY 0x23A7
#define BTP_DATA_LEN 0x8
#define BTP_CAP_PER 100
#define BTP_INIT_WAIT 20
#define BTP_NUM_OF_ATMPT 5

/* private bus/device/non-isr interface.*/
struct bq40z40_interface {
	void *i2c_dev;
};

static int bq40z40_mfg_read(void *device, uint16_t cmd_code,
			    uint8_t *rx_data, uint8_t len)
{
	int result = FAILURE;
	uint8_t command = BQ40Z40_MFG_BLOCK_ACCESS;
	uint8_t addr[BTP_MFG_RD_SIZE] = { 0x0 };

	addr[0] = BTP_MFG_ADDR;
	addr[1] = BTP_MFG_ADDR_MASK & cmd_code;
	addr[2] = cmd_code >> BTP_MFG_ADDR_SHIFT;

	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	result = eclite_i2c_burst_write(dev_interface->i2c_dev,
					battery_data->sbs_i2c_slave_addr,
					command, &addr[0], BTP_MFG_RD_SIZE);

	if (result != SUCCESS) {
		ECLITE_LOG_DEBUG("%s(): Failed to read manufacturer block\n",
				 __func__);
		return result;
	}

	result = eclite_i2c_burst_read(dev_interface->i2c_dev,
				       battery_data->sbs_i2c_slave_addr,
				       command, rx_data, len + BTP_MFG_RD_SIZE);

	/* Remove address bytes */
	memmove(rx_data, &rx_data[BTP_MFG_RD_SIZE], len);

	return result;
}

static int bq40z40_mfg_write(void *device, uint16_t cmd_code,
			     uint8_t *tx_data, uint8_t len)
{
	int result = FAILURE;
	uint8_t command = BQ40Z40_MFG_BLOCK_ACCESS;
	uint8_t tx_buf[BQ40Z40_MFG_DATA_BUF_SIZE + BTP_MFG_WR_SIZE] = { 0x0 };
	uint8_t addr[BTP_MFG_WR_SIZE] = { 0x0 };

	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	addr[0] = BTP_MFG_ADDR;
	addr[1] = BTP_MFG_ADDR_MASK & cmd_code;
	addr[2] = cmd_code >> BTP_MFG_ADDR_SHIFT;

	/* Copy address and data in the tx buffer */
	memcpy(tx_buf, addr, BTP_MFG_WR_SIZE);
	memcpy(&tx_buf[BTP_MFG_WR_SIZE], tx_data, len);

	result = eclite_i2c_burst_write(dev_interface->i2c_dev,
					battery_data->sbs_i2c_slave_addr,
					command, &tx_buf[0],
					len + BTP_MFG_WR_SIZE);

	if (result != SUCCESS) {
		ECLITE_LOG_DEBUG("%s(): Failed to write manufacturer block\n",
				 __func__);
		return result;
	}

	return result;
}

static int bq40z40_enable_btp(void *device)
{
	int result = SUCCESS;
	uint8_t data[BTP_EN_BUF_SIZE] = { 0 };
	uint16_t io_config_reg = BQ40Z40_DATAFLASH_IO_CONFIG;

	bq40z40_mfg_read(device, io_config_reg, &data[0], BTP_EN_RD_SIZE);

	/* Verify BTP enabled */
	if (data[0] & BQ40Z40_BTP_EN) {
		ECLITE_LOG_DEBUG("%s(): BTP enabled\n", __func__);
	} else {
		/* BTP not enabled! Enable BTP now */
		data[0] |= BQ40Z40_BTP_EN;
		result = bq40z40_mfg_write(device, io_config_reg, &data[0],
					   BTP_EN_RD_SIZE);
		if (result == SUCCESS) {
			ECLITE_LOG_DEBUG("%s(): BTP enabled successfully\n",
					 __func__);
		} else {
			ECLITE_LOG_DEBUG("%s(): BTP enable failed\n", __func__);
		}
	}

	return result;
}

/* unlock battery. This function call should happen if battery is detected.
 * NOTE: Below unlock function is supported with only battery part J66644-004.
 */
static int bq40z40_unlock(void *device)
{
	int result = SUCCESS;
	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;
	uint8_t data[BTP_DATA_LEN] = { 0 };
	uint16_t unlock_key = BTP_EN_KEY;

	if (bq40z40_mfg_read(device, BQ40Z40_MFG_OP_STATUS, &data[0],
			     BTP_EN_BUF_SIZE) != SUCCESS) {
		ECLITE_LOG_DEBUG("Failed to read manufacturer block\n");
		return FAILURE;
	}

	if (data[BTP_MFG_RD_SIZE] & BQ40Z40_EM_SHUT) {
		ECLITE_LOG_DEBUG("%s(): Battery locked! Unlocking now\n",
				 __func__);
		result = eclite_i2c_write_word(dev_interface->i2c_dev,
					       battery_data->sbs_i2c_slave_addr,
					       BQ40Z40_MFG_ACCESS,
					       unlock_key);
	} else {
		ECLITE_LOG_DEBUG("%s(): Battery unlocked\n", __func__);
	}

	return result;
}

/* Charger framework callback to read the FG data. */
static int bq40z40_at_rate(void *device, uint16_t *rate)
{
	uint16_t ret;
	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	ret = eclite_i2c_read_word((void *)dev_interface->i2c_dev,
				   battery_data->sbs_i2c_slave_addr,
				   BQ40Z40_CURRENT_REG,
				   rate);

	/* Verify current is negative, if negative convert to positive */
	if (*rate & CHECK_NEGATIVE_BIT) {
		*rate = ~(*rate) + 1;
	}

	ECLITE_LOG_DEBUG("Battery rate: %x", *rate);
	return ret;

}

static int bq40z40_design_voltage(void *device, uint16_t *design_voltage)
{
	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	return (eclite_i2c_read_word((void *)dev_interface->i2c_dev,
				     battery_data->sbs_i2c_slave_addr,
				     BQ40Z40_DESIGN_VOLTAGE_REG,
				     design_voltage));

}

static int bq40z40_get_battery_voltage(void *device, uint16_t *voltage)
{
	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	return (eclite_i2c_read_word((void *)dev_interface->i2c_dev,
				     battery_data->sbs_i2c_slave_addr,
				     BQ40Z40_VOLTAGE_REG,
				     voltage));
}

static int bq40z40_get_battery_current(void *device, uint16_t *current)
{
	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	return (eclite_i2c_read_word((void *)dev_interface->i2c_dev,
				     battery_data->sbs_i2c_slave_addr,
				     BQ40Z40_CURRENT_REG,
				     current));
}

static int bq40z40_get_battery_charge(void *device,
				      uint16_t *absolute_charge)
{
	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	return (eclite_i2c_read_word(dev_interface->i2c_dev,
				     battery_data->sbs_i2c_slave_addr,
				     BQ40Z40_ABSOLUTE_STATE_OF_CHARGE_REG,
				     absolute_charge));
}

static int bq40z40_get_battery_relative_charge(void *device,
					       uint16_t *relative_charge)
{
	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	return (eclite_i2c_read_word(dev_interface->i2c_dev,
				     battery_data->sbs_i2c_slave_addr,
				     BQ40Z40_RELATIVE_STATE_OF_CHARGE_REG,
				     relative_charge));
}

static int bq40z40_get_battery_full_charge_capacity(void *device,
						uint16_t *full_charge_capacity)
{
	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	return (eclite_i2c_read_word(dev_interface->i2c_dev,
				     battery_data->sbs_i2c_slave_addr,
				     BQ40Z40_FULL_CHARGE_CAPACITY_REG,
				     full_charge_capacity));
}

static int bq40z40_get_remaining_battery(void *device,
					 uint16_t *remaining_capacity)
{
	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	return (eclite_i2c_read_word(dev_interface->i2c_dev,
				     battery_data->sbs_i2c_slave_addr,
				     BQ40Z40_REMAINING_BATTERY_REG,
				     remaining_capacity));
}

static int bq40z40_get_battery_charge_voltage(void *device,
					      uint16_t *charge_voltage)
{
	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	return (eclite_i2c_read_word(dev_interface->i2c_dev,
				     battery_data->sbs_i2c_slave_addr,
				     BQ40Z40_CHARGE_VOLTAGE_REG,
				     charge_voltage));
}

static int bq40z40_get_battery_charge_current(void *device,
					      uint16_t *charge_current)
{
	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	return (eclite_i2c_read_word(dev_interface->i2c_dev,
				     battery_data->sbs_i2c_slave_addr,
				     BQ40Z40_CHARGE_CURRENT_REG,
				     charge_current));
}


static int bq40z40_get_battery_design_capacity(void *device,
					       uint16_t *design_capacity)
{
	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	return (eclite_i2c_read_word(dev_interface->i2c_dev,
				     battery_data->sbs_i2c_slave_addr,
				     BQ40Z40_DESIGN_CAPACITY_REG,
				     design_capacity));
}

static int bq40z40_get_battery_cycle_count(void *device,
					   uint16_t *cycle_count)
{
	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	return (eclite_i2c_read_word(dev_interface->i2c_dev,
				     battery_data->sbs_i2c_slave_addr,
				     BQ40Z40_CYCLE_COUNT_REG,
				     cycle_count));
}

static int bq40z40_set_battery_trip_point(void *device,
					  uint16_t *trip_value)
{
	int result = 0;
	uint8_t delta = 0;
	uint16_t btp_chg_val = 0;
	uint16_t btp_dischg_val = 0;
	uint16_t fullcharge_capacity = 0;
	uint16_t remaining_capacity = 0;

	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	if (trip_value == NULL) {
		ECLITE_LOG_DEBUG("%s(): BTP setting failed\n", __func__);
		return ERROR;
	}

	btp_dischg_val = *trip_value;
	btp_chg_val    = *trip_value;

	if (*trip_value == 0) {
		result = eclite_i2c_read_word(dev_interface->i2c_dev,
					      battery_data->sbs_i2c_slave_addr,
					      BQ40Z40_FULL_CHARGE_CAPACITY_REG,
					      &fullcharge_capacity);

		result |= eclite_i2c_read_word(dev_interface->i2c_dev,
					       battery_data->sbs_i2c_slave_addr,
					       BQ40Z40_REMAINING_BATTERY_REG,
					       &remaining_capacity);
		if (result == SUCCESS) {
			delta = fullcharge_capacity / BTP_CAP_PER;
			btp_dischg_val = remaining_capacity - delta;
			btp_chg_val = remaining_capacity + delta;
		}
	}

	result = eclite_i2c_write_word(dev_interface->i2c_dev,
				       battery_data->sbs_i2c_slave_addr,
				       BQ40Z40_BTP_DISCHARGE,
				       btp_dischg_val);

	result |= eclite_i2c_write_word(dev_interface->i2c_dev,
					battery_data->sbs_i2c_slave_addr,
					BQ40Z40_BTP_CHARGE,
					btp_chg_val);

	return result;
}

static int bq40z40_get_battery_status(void *device,
				      uint16_t *battery_status)
{
	struct eclite_device *dev = device;
	struct sbs_driver_data *battery_data = dev->driver_data;
	struct bq40z40_alarm_warning_reg warning = { 0 };
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;

	eclite_i2c_read_word(dev_interface->i2c_dev,
			     battery_data->sbs_i2c_slave_addr,
			     BQ40Z40_ALARM_WARNING_REG,
			     (uint16_t *)&warning);

	return (warning.over_temprature_alarm |
		warning.terminate_discharge_alarm |
		warning.over_charged_alarm_alarm);
}

static int bq40z40_check_battery_presence(void *bq40z40_eclite_device)
{
	int ret;
	uint16_t voltage;

	/* battery read failure indicates battery is not present */
	ret = bq40z40_get_battery_voltage(bq40z40_eclite_device, &voltage);
	if (ret) {
		LOG_WRN("Battery not present");
		return ret;
	}

	return 0;
}

static enum device_err_code bq40z40_isr(void *bq40z40_eclite_device)
{
	struct eclite_device *bq40z40_eclite_dev = bq40z40_eclite_device;

	ARG_UNUSED(bq40z40_eclite_dev);
	/* isr handling*/
	return SUCCESS;
}

/* Initialize FG. */
static enum device_err_code bq40z40_init(void *bq40z40_device)
{
	int result = 0x0;
	uint16_t btp_value = 0x0;
	uint16_t fgstat = 0x0;
	uint8_t numofattempts = 0x0;
	struct eclite_device *dev = bq40z40_device;
	struct bq40z40_interface *dev_interface = dev->hw_interface->device;
	struct sbs_driver_data *battery_data = dev->driver_data;

	ECLITE_LOG_DEBUG("Battery driver init");

	dev_interface->i2c_dev = (struct device *)
				 eclite_get_i2c_device(ECLITE_I2C_DEV);
	dev->hw_interface->gpio_dev = (struct device *)
				      eclite_get_gpio_device(BATTERY_GPIO_NAME);

	if (dev_interface->i2c_dev == NULL) {
		LOG_ERR("Battery device init failure");
		return DEV_INIT_FAILED;
	}

	eclite_i2c_config(
		dev_interface->i2c_dev,
		(I2C_SPEED_STANDARD <<
		 I2C_SPEED_SHIFT) | I2C_MODE_MASTER);

	/* Verify FG configured or not */
	do {
		eclite_i2c_read_word(dev_interface->i2c_dev,
				     battery_data->sbs_i2c_slave_addr,
				     BQ40Z40_BAT_STATUS,
				     &fgstat);

		/* INIT bit sets when FG Battery configured successfully */
		if (fgstat & BQ40Z40_BAT_INIT_DONE) {
			ECLITE_LOG_DEBUG("%s(): FG configured\n", __func__);
			break;
		}
		/* 20msec delay */
		k_sleep(K_MSEC(BTP_INIT_WAIT));
		numofattempts++;
	} while (numofattempts < BTP_INIT_WAIT);

	if (numofattempts == BTP_INIT_WAIT) {
		/* FG failed to initialize even after 100msecs */
		ECLITE_LOG_DEBUG("%s(): FG init failed! Don't config FG\n",
				 __func__);
		return FAILURE;
	}

	bq40z40_unlock(bq40z40_device);
	bq40z40_enable_btp(bq40z40_device);

	result = bq40z40_check_battery_presence(bq40z40_device);
	if (result == SUCCESS) {
		bq40z40_set_battery_trip_point(bq40z40_device, &btp_value);
	}
	return result;
}

/* Battery Fuelguage interface APIs. */
static APP_GLOBAL_VAR(1) struct sbs_driver_api bq40z40_api = {
	.voltage = bq40z40_get_battery_voltage,
	.current = bq40z40_get_battery_current,
	.absolute_state_of_charge = bq40z40_get_battery_charge,
	.relative_state_of_charge = bq40z40_get_battery_relative_charge,
	.full_charge_capacity    = bq40z40_get_battery_full_charge_capacity,
	.remaining_capacity = bq40z40_get_remaining_battery,
	.charging_voltage = bq40z40_get_battery_charge_voltage,
	.charging_current = bq40z40_get_battery_charge_current,
	.design_capacity = bq40z40_get_battery_design_capacity,
	.cycle_count = bq40z40_get_battery_cycle_count,
	.battery_status = bq40z40_get_battery_status,
	.battery_trip_point = bq40z40_set_battery_trip_point,
	.check_battery_presence = bq40z40_check_battery_presence,
	.at_rate = bq40z40_at_rate,
	.design_voltage = bq40z40_design_voltage,
};


/* Battery Fuelguage interface Data. */
static APP_GLOBAL_VAR_BSS(1) struct sbs_driver_data bq40z40_data;

/* device private interface */
static APP_GLOBAL_VAR_BSS(1) struct bq40z40_interface bq40z40_device;

/* HW interface configuration for battery. */
static APP_GLOBAL_VAR(1) struct hw_configuration bq40z40_hw_interface = {
	.device = &bq40z40_device,
};

/* Device structure for bq40z40. */
APP_GLOBAL_VAR(1) struct eclite_device bq40z40_device_fg = {
	.name = "BQ40Z40_device_FG",
	.device_typ = DEV_FG,
	.init = bq40z40_init,
	.driver_api = &bq40z40_api,
	.driver_data = &bq40z40_data,
	.isr = bq40z40_isr,
	.hw_interface = &bq40z40_hw_interface,
};

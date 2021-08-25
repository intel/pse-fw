/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <eclite_device.h>
#include "eclite_device.h"
#include "ucsi.h"
#include "common.h"
#include "eclite_hw_interface.h"
#include "ccg.h"
#include <logging/log.h>
#include <platform.h>

LOG_MODULE_REGISTER(ccg, CONFIG_ECLITE_LOG_LEVEL);

/* CCG controller HPI register definitions */
#define CCG_HPI_DEVICE_MODE_REG         0x0000
#define DEVICE_MODE_MASK                0x03
#define DEVICE_MODE_REG_FLAG_BOOT       0x0
#define DEVICE_MODE_REG_FLAG_FW1        0x01
#define DEVICE_MODE_REG_FLAG_FW2        0x02

#define CCG_HPI_INTERRUPT_REG           0X0006
#define CCG_DEVICE_INTERRUPT_FLAG       BIT0
#define CCG_PORT0_INTERRUPT_FLAG        BIT1
#define CCG_PORT1_INTERRUPT_FLAG        BIT2
#define CCG_UCSIREAD_INTERRUPT_FLAG     BIT7

#define CCG_HPI_RESET_REG               0X0008

#define CCG_HPI_READ_ALL_VERSIONS       0x0010
#define CCG_FW2_VERSION_REG             0x0020
#define CCG_HPI_PDPORT_ENABLE_REG       0X002C
#define PDPORT_ENABLE_REG_FLAG_PORT0    BIT0
#define PDPORT_ENABLE_REG_FLAG_PORT1    BIT1

#define CCG_HPI_UCSI_STATUS_REG                 0x0038
#define CCG_HPI_SYS_PWR_ST_REG                  0x003B
#define UCSI_STATUS_REG_FLAG_UCSI_STARTED       BIT0
#define UCSI_STATUS_REG_FLAG_CONNAND_IP         BIT1
#define UCSI_STATUS_REG_FLAG_EVENT_PENDING      BIT2

#define CCG_HPI_UCSI_CONTROL_REG                        0x0039
#define UCSI_CONTROL_REG_FLAG_START_UCSI                BIT0
#define UCSI_CONTROL_REG_FLAG_STOP_UCSI                 BIT1
#define UCSI_CONTROL_REG_FLAG_SILENCE_UCSI              BIT2
#define UCSI_CONTROL_REG_FLAG_SIGNAL_CONNECT_EVENT      BIT0

#define CCG_HPI_RESPONSE_REG            0X007E
#define RESPONSE_REG_FLAG_TYPE          BIT7
#define RESPONSE_REG_FLAG_MESSAGE       0X7F

#define CCG_HPI_PORT_BASE               0x1000
#define CCG_HIP_REG_ADDR(p, o)          (0x1000 * p + CCG_HPI_PORT_BASE + o)

#define CCG_HPI_PDRESPONSE_REG(x)       CCG_HIP_REG_ADDR(x, 0X0400)
#define CCG_HPI_PDRESPONSE_DATA_REG(x)  CCG_HIP_REG_ADDR(x, 0X0404)

#define CCG_HPI_EVENT_MASK_REG(x)       CCG_HIP_REG_ADDR(x, 0X0024)

#define RESPONSE_CODE_NO_RESPONSE       0x00
#define RESPONSE_CODE_CMD_SUCCESS       0x02
#define RESPONSE_CODE_CMD_FAILED_UCSI   0x15
#define RESPONSE_CODE_TYPEC_CONNECT     0x84
#define RESPONSE_CODE_TYPEC_DISCONNECT  0x85
#define RESPONSE_CODE_PD_NEGO_COMPLETE  0x86
#define RESPONSE_CODE_PD_CNTRL_MSG      0x87

#define RESPONSE_SWAP_TYPE_BITS         (BIT0 | BIT1 | BIT2 | BIT3)

#define RESPONSE_SWAP_TYPE_DR_SWAP      0x0
#define RESPONSE_SWAP_TYPE_PR_SWAP      0x1
#define RESPONSE_SWAP_TYPE_VCON_SWAP    0x2

#define RESPONSE_SWAP_STSTUS_BITS       (BIT4 | BIT5 | BIT6 | BIT7)

#define RESPONSE_SWAP_STSTUS_SUCCESS    0x0
#define RESPONSE_SWAP_STSTUS_REJECT     0x1
#define RESPONSE_SWAP_STSTUS_WAIT       0x2
#define RESPONSE_SWAP_STSTUS_NORESPONSE 0x3
#define RESPONSE_SWAP_STSTUS_HARDRESET  0x4

#define CCG_HPI_PD_STATUS_REG(x)        CCG_HIP_REG_ADDR(x, 0X0008)
#define CCG_HPI_TYPEC_STATUS_REG(x)     CCG_HIP_REG_ADDR(x, 0X000C)
#define CCG_HPI_CURRENT_PDO(x)          CCG_HIP_REG_ADDR(x, 0X0010)

#define REG_CCG_FORCE_TBT_MODE          0X0040
/*(21 * 5)105ms dealy for reading any response*/
#define CCG_FORCE_TBT_MODE_ENTRY_TIME   21
#define CCG_CTRL0_I2C_WR_STS_MASK       BIT0
#define CCG_CTRL0_CMD_ACK_STS_MASK      BIT1
#define CCG_CTRL1_I2C_WR_STS_MASK       BIT4
#define CCG_CTRL1_CMD_ACK_STS_MASK      BIT5

/* CCG controller HPI register definitions */
#define CCG_UCSI_VERSION_REG            0xF000
#define CCG_UCSI_CCI_REG                0xF004
#define CCG_UCSI_CONTROL_REG            0xF008
#define CCG_UCSI_MSG_IN                 0xF010
#define CCG_UCSI_MSG_OUT                0xF020

#define N_PORT                          0x2
#define SINK_ATTACHED                   0x1

#define HIGH    1
#define LOW     0
#define COUNTER_TIME 20
#define WAIT_TIME 5
#define CMD_CODE 0x7
#define PARTNER_ATTACH_CHK 2
#define VERSION_REG 8

#define RD_LEN1 1
#define RD_LEN2 2
#define RD_LEN4 4
#define RD_LEN8 8
#define RD_LEN16 16

/* private bus/device/non-isr interface.*/
struct ccg_interface {
	void *i2c_dev;
	uint8_t i2c_slave_addr;
};

static APP_GLOBAL_VAR(1) uint8_t version[VERSION_REG];

static APP_GLOBAL_VAR(1) struct ccg_port_state {
	uint8_t data_role;
	uint8_t pwr_role;
	uint8_t partner_attached;
	uint8_t connection_state;
} state[N_PORT];

static void get_port_state(uint8_t port, uint8_t cmd, uint8_t len, uint8_t *buf)
{
	switch (cmd) {
	case DATA_ROLE:
		*buf = state[port].data_role;
		break;
	case POWER_ROLE:
		*buf = state[port].pwr_role;
		break;
	case PARTNER_ATTACHED:
		*buf = state[port].partner_attached;
		break;
	case CONNECTION_STATUS:
		*buf = state[port].connection_state;
		break;
	case PD_VERSION:
		memcpy(buf, &version[0], len);
		break;
	default:
		break;
	}
}

static int ccg_wr(void *pd_dev, uint16_t reg, uint8_t *ptr, size_t length)
{
	struct eclite_device *dev = pd_dev;
	struct ccg_interface *dev_interface = dev->hw_interface->device;

	if (dev_interface->i2c_dev == NULL) {
		LOG_ERR("No CCG I2C device");
		return ERROR;
	}

	return eclite_i2c_burst_write16(dev_interface->i2c_dev,
					dev_interface->i2c_slave_addr,
					reg, ptr, length);
}

static int ccg_rd(void *pd_dev, uint16_t reg, uint8_t *ptr, size_t length)
{
	struct eclite_device *dev = pd_dev;
	struct ccg_interface *dev_interface = dev->hw_interface->device;

	if (dev_interface->i2c_dev == NULL) {
		return ERROR;
	}

	return eclite_i2c_burst_read16(dev_interface->i2c_dev,
				       dev_interface->i2c_slave_addr,
				       reg, ptr, length);
}

static void ccg_clear_interrupt(void *dev, uint8_t val)
{
	uint8_t ret;

	ret = ccg_wr(dev, CCG_HPI_INTERRUPT_REG, &val, RD_LEN1);
	ECLITE_LOG_DEBUG("CCG_HPI_INTERRUPT_REG ret: %d, val: %d", ret, val);
}

static int  ccg_block_for_int(struct eclite_device *dev)
{
	uint8_t count = COUNTER_TIME; /* 5 * 20 = 100 msec */
	uint8_t ret;
	uint8_t buf = 0;
	uint32_t temp;

	while (count) {
		if (LOW ==
		    (eclite_get_gpio(dev->hw_interface->gpio_dev, UCSI_GPIO,
				     &temp))) {
			ret = ccg_rd(dev, CCG_HPI_INTERRUPT_REG, &buf, RD_LEN1);
			ECLITE_LOG_DEBUG("CCG HPI Intr Reg ret: %x, buf: %x",
					 ret, buf);
			if (ret) {
				return 0; /* Return 0 means no interrupts */
			}
			return buf;
		}
		k_sleep(K_MSEC(WAIT_TIME));
		count++;
	}
	ECLITE_LOG_DEBUG("Wait for interrupt timedout");

	return 0; /* Return 0 means no interrupts */

}

static int init_ucsi(void *pd_device)
{
	uint8_t buf[RD_LEN2] = { 0 };
	uint8_t ucsistatus = 0;
	int ret;
	uint32_t temp;
	struct eclite_device *dev = pd_device;

	ret =  eclite_get_gpio(dev->hw_interface->gpio_dev, UCSI_GPIO, &temp);

	if (temp == LOW) {
		ccg_clear_interrupt(dev, 0xff);
		ECLITE_LOG_DEBUG("Clear interrupts");
	}

	/* Enable UCSI interface */
	buf[0] = UCSI_CONTROL_REG_FLAG_START_UCSI;
	ret = ccg_wr(dev, CCG_HPI_UCSI_CONTROL_REG, buf, RD_LEN1);
	if (ret) {
		goto ucsi_init_error;
	}

	ret = ccg_block_for_int(dev);
	if ((ret & CCG_DEVICE_INTERRUPT_FLAG) == 0) {
		goto ucsi_init_error;
	}

	ret = ccg_rd(dev, CCG_HPI_RESPONSE_REG, buf, RD_LEN2);
	if (ret) {
		goto ucsi_init_error;
	}

	if (((buf[0] & RESPONSE_REG_FLAG_TYPE) == 0)
	    && ((buf[0] & RESPONSE_REG_FLAG_MESSAGE) !=
		RESPONSE_CODE_CMD_SUCCESS)
	    && ((buf[0] & RESPONSE_REG_FLAG_MESSAGE) !=
		RESPONSE_CODE_CMD_FAILED_UCSI)) {
		LOG_ERR("UCSI Start failed: %x", buf[0]);
		goto ucsi_init_error;
	}

	ret = ccg_rd(dev, CCG_HPI_UCSI_STATUS_REG, &ucsistatus, RD_LEN1);
	ECLITE_LOG_DEBUG("CCG_HPI_UCSI_STATUS_REG  ret: %d, ucsistatus: %d",
			 ret, ucsistatus);
	if (ret) {
		goto ucsi_init_error;
	}

	if (((buf[0] & RESPONSE_REG_FLAG_MESSAGE) ==
	     RESPONSE_CODE_CMD_FAILED_UCSI)
	    && ((ucsistatus & UCSI_STATUS_REG_FLAG_UCSI_STARTED) !=
		UCSI_STATUS_REG_FLAG_UCSI_STARTED)) {
		LOG_ERR("UCSI Start failed");
		goto ucsi_init_error;
	}

	/* Clear all interrupt*/
	ccg_clear_interrupt(dev, 0xFF);

	ret = ccg_rd(dev, CCG_UCSI_VERSION_REG, buf, RD_LEN2);
	if (ret) {
		goto ucsi_init_error;
	}

	/* ccg_handle_cntrl_int(Cntrl);*/
	ECLITE_LOG_DEBUG("CCG UCSI init successful");
	return 0;

ucsi_init_error:
	LOG_ERR("CCG UCSI init failed");
	return ERROR;
}

static int ucsi_cmd_from_os(void *ccg_device)
{
	uint8_t ret;
	uint8_t buf = 0;
	struct eclite_device *dev = find_dev_by_type(DEV_UCSI);

	/* Look for USCI device , if not found return error */
	if (!dev) {
		LOG_ERR("No UCSI device");
		return ERROR;
	}

	struct ccg_interface *dev_interface = dev->hw_interface->device;

	/* Look for dev_interface, if not found return error */
	if (!dev_interface) {
		return ERROR;
	}

	struct ucsi_data_t *data = dev->driver_data;

	/* Look for data, if not found return error */
	if (!data) {
		LOG_ERR("No UCSI data");
		return ERROR;
	}

	ECLITE_LOG_DEBUG("Command received from OS");

	if (data->control.command == CMD_CODE) {
		ECLITE_LOG_DEBUG("Connector Capability (Port): %x",
				 data->control.cmdspecific[0]);
	}

	/*Check for command in progress.*/
	ret = ccg_rd(dev, CCG_HPI_UCSI_STATUS_REG, &buf, sizeof(buf));
	if (ret) {
		LOG_ERR("CCG STATUS read failed");
		return ret;
	}

	if (buf & UCSI_STATUS_REG_FLAG_CONNAND_IP) {
		LOG_ERR("Previous command pending");
		return ERROR;
	}

	/* Write control and MessageOut to Controller from local memory */
	ret = ccg_wr(dev, CCG_UCSI_MSG_OUT,
		     (uint8_t *)&data->msg_out, sizeof(data->msg_out));
	if (ret) {
		LOG_ERR("CCG MSG OUT write failed");
		return ret;
	}

	ret = ccg_wr(dev, CCG_UCSI_CONTROL_REG,
		     (uint8_t *)&data->control, sizeof(data->control));
	if (ret) {
		LOG_ERR("CCG CONTROL write failed");
		return ret;
	}

	return 0;
}

static int ucsi_int_handler(struct eclite_device *ccg_device)
{
	struct eclite_device *dev = ccg_device;
	struct ucsi_data_t *data = dev->driver_data;
	int ret;

	ECLITE_LOG_DEBUG("UCSI intr handler");

	/* Read CCI from Controller to local memory */
	ret = ccg_rd(dev, CCG_UCSI_CCI_REG, (uint8_t *)&data->cci,
		     sizeof(data->cci));
	if (ret) {
		LOG_ERR("CCG CCI read failed");
		return ERROR;
	}

	ret = ccg_rd(dev, CCG_UCSI_MSG_IN, (uint8_t *)&data->msg_in,
		     sizeof(data->msg_in));
	if (ret) {
		LOG_ERR("CCG MSG IN read failed");
		return ERROR;
	}

	return 0;
}

static int device_int_handler(struct eclite_device *dev)
{
	ECLITE_LOG_DEBUG("Device intr generated");
	return 0;
}

static int process_port_evnts(struct eclite_device *dev,
			      uint8_t port, uint8_t code)
{
	uint8_t ret = 0;
	uint8_t buf[RD_LEN8] = { 0 };

	switch (code) {
	case RESPONSE_CODE_TYPEC_CONNECT:
		ret = ccg_rd(dev, CCG_HPI_TYPEC_STATUS_REG(
				     port), &buf[0], RD_LEN1);
		if (ret) {
			LOG_ERR("CCG TYPEC STATUS read failed");
			break;
		}
		if ((buf[0] & BIT0) == 0) {
			LOG_ERR("Spurious connect event");
			break;
		}
		state[port].partner_attached =
			(buf[0] & (BIT2 | BIT3 | BIT4)) >> PARTNER_ATTACH_CHK;
		if (state[port].partner_attached == SINK_ATTACHED) {
			ECLITE_LOG_DEBUG("TypeC sink connected");
			state[port].pwr_role = false;
			state[port].data_role = false;
		}
		break;

	case RESPONSE_CODE_TYPEC_DISCONNECT:
		ECLITE_LOG_DEBUG("TypeC disconnected");
		state[port].connection_state = false;
		state[port].data_role = false;
		break;
	default:
		ECLITE_LOG_DEBUG("Unknown event: %x", code);
		break;
	}
	return ret;
}

static int port_int_handler(struct eclite_device *dev, uint8_t port)
{
	uint8_t buf[RD_LEN4] = { 0 };
	uint8_t ret;
	uint16_t offset;

	ECLITE_LOG_DEBUG("Port interrupt: %x", port);

	offset = CCG_HPI_PDRESPONSE_REG(port);
	ret = ccg_rd(dev, offset, buf, RD_LEN4);
	if (ret) {
		LOG_ERR("CCG PD RESPONSE read failed");
		return ret;
	}

	if (buf[0] & RESPONSE_REG_FLAG_TYPE) {
		process_port_evnts(dev, port, buf[0]);
	}

	return 0;
}

static enum device_err_code ccg_isr(void *ccg_device)
{
	uint8_t ret;
	uint8_t buf = 0;
	struct eclite_device *dev = find_dev_by_type(DEV_UCSI);

	/* Look for USCI device, if not found return error */
	if (!dev) {
		LOG_ERR("No UCSI device");
		return ERROR;
	}

	struct ccg_interface *dev_interface = dev->hw_interface->device;

	/* Look for dev_interface, if not found return error */
	if (!dev_interface) {
		LOG_ERR("No UCSI device interface");
		return ERROR;
	}

	ECLITE_LOG_DEBUG(" ");

	/* read interrupt source */
	ret = ccg_rd(dev, CCG_HPI_INTERRUPT_REG, &buf, RD_LEN1);
	if (ret) {
		LOG_WRN("CCG read failed");
		return ERROR;
	}

	/* process interrupt */
	if ((buf & CCG_DEVICE_INTERRUPT_FLAG)) {
		ret = device_int_handler(dev);
	}

	if ((buf & CCG_PORT0_INTERRUPT_FLAG)) {
		ret = port_int_handler(dev, 0);
	}

	if ((buf & CCG_UCSIREAD_INTERRUPT_FLAG)) {
		ret = ucsi_int_handler(dev);
	}

	if (ret) {
		LOG_ERR("Handle CCG interrupt failed");
		return ret;
	}

	/* clear interrupt */
	ret = ccg_wr(dev, CCG_HPI_INTERRUPT_REG, &buf, RD_LEN1);

	if (ret) {
		LOG_ERR("CCG clear interrupt failed");
		return ret;
	}

	return ret;
}

static int ccg_i2c_reset(struct eclite_device *dev)
{
	unsigned char buf[RD_LEN2] = { 0 };
	uint8_t ret;

	/* Read DEVICE_MODE reg to know that controller is ready */
	ret = ccg_rd(dev, CCG_HPI_DEVICE_MODE_REG, buf, RD_LEN1);
	if (ret) {
		LOG_ERR("Device mode read failed");
		return ERROR;
	}

	if (buf[0] & DEVICE_MODE_MASK) {
		ECLITE_LOG_DEBUG("Success");
		return SUCCESS;
	}

	LOG_WRN("Reset failed");
	return ERROR;

}

static int init_chip(struct eclite_device *pd_device)
{
	uint8_t buf[RD_LEN16];
	uint8_t int_status;
	int ret;
	struct eclite_device *dev = pd_device;

	ECLITE_LOG_DEBUG("CCG initialize chip");

	ret = ccg_rd(dev, CCG_HPI_INTERRUPT_REG, &int_status, RD_LEN1);

	/* Clear interrupt reg */
	ccg_clear_interrupt(dev, 0xFF);
	ECLITE_LOG_DEBUG("Clear interrupts");

	ret = ccg_wr(dev, CCG_HPI_EVENT_MASK_REG(0), buf, RD_LEN4);
	if (ret) {
		goto chip_init_error;
	}

	/* Reset I2c Interface of the chip, which clear all previous event.*/
	ret = ccg_i2c_reset(dev);
	if (ret) {
		goto chip_init_error;
	}

	/* Read FW Version register */
	ret = ccg_rd(dev, CCG_FW2_VERSION_REG, &version[0], RD_LEN8);
	if (ret) {
		goto chip_init_error;
	}

	/* Enable PD Ports per controller */
	buf[0] = PDPORT_ENABLE_REG_FLAG_PORT0;

	ret = ccg_wr(dev, CCG_HPI_PDPORT_ENABLE_REG, buf, RD_LEN1);
	if (ret) {
		goto chip_init_error;
	}

	ret = ccg_block_for_int(dev);
	ECLITE_LOG_DEBUG("Block ret: %d", ret);
	if ((ret & CCG_DEVICE_INTERRUPT_FLAG) == 0) {
		goto chip_init_error;
	}

	ret = ccg_rd(dev, CCG_HPI_RESPONSE_REG, buf, RD_LEN2);
	if (ret) {
		goto chip_init_error;
	}

	/* Clear interrupt reg */
	ccg_clear_interrupt(dev, 0xFF);
	ECLITE_LOG_DEBUG("Port enable resp: %x", buf[0]);

	if (((buf[0] & RESPONSE_REG_FLAG_TYPE) == 0)
	    && ((buf[0] & RESPONSE_REG_FLAG_MESSAGE) !=
		RESPONSE_CODE_CMD_SUCCESS)) {
		LOG_ERR("PDPORT enable failed: %x", buf[0]);
		goto chip_init_error;
	}

	/* Enable Typec connect, disconnect, PD negotiation complete,
	 * PD control Msgs event notification
	 */
	buf[0] = 0x38;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 0;

	ret = ccg_wr(dev, CCG_HPI_EVENT_MASK_REG(0), buf, RD_LEN4);
	if (ret) {
		goto chip_init_error;
	}

	ECLITE_LOG_DEBUG("CCG initialize chip successful");
	return 0;

chip_init_error:
	ECLITE_LOG_DEBUG("CCG initialize chip failed");
	return 1;

}

static enum device_err_code ccg_pd_init(void *pd_device)
{
	struct eclite_device *dev = pd_device;
	struct ccg_interface *dev_interface = dev->hw_interface->device;
	int ret;

	ECLITE_LOG_DEBUG("PD Controller driver init");

	dev_interface->i2c_dev = (struct device *)
				 eclite_get_i2c_device(ECLITE_UCSI_I2C_DEV);
	dev->hw_interface->gpio_dev = (struct device *)
				      eclite_get_gpio_device(UCSI_GPIO_NAME);

	if (dev_interface->i2c_dev == NULL) {
		LOG_ERR("CCG device init failed");
		return DEV_INIT_FAILED;
	}

	eclite_i2c_config(
		dev_interface->i2c_dev,
		(I2C_SPEED_STANDARD <<
		 I2C_SPEED_SHIFT) | I2C_MODE_MASTER);

	ret = init_chip(pd_device);
	if (ret) {
		LOG_WRN("Chip init failed");
		return ERROR;
	}

	ret = init_ucsi(pd_device);
	if (ret) {
		LOG_ERR("PD init failed");
		return ret;
	}

	ECLITE_LOG_DEBUG("PD init Success");
	return 0;
}

/* ucsi interface APIs. */
static APP_GLOBAL_VAR(1) struct ucsi_api_t ccg_api = {
	.ucsi_fn = ucsi_cmd_from_os,
	.get_pd_state = get_port_state,
};

/* ucsi  interface Data. */
static APP_GLOBAL_VAR_BSS(1) struct ucsi_data_t ccg_data;

/* device private interface */
static APP_GLOBAL_VAR(1) struct ccg_interface ccg_device = {
	.i2c_slave_addr = UCSI_I2C_SLAVE_ADDRESS,
};

/* HW interface configuration for battery. */
static APP_GLOBAL_VAR(1) struct hw_configuration ccg_hw_interface = {
	.device = &ccg_device,
};

APP_GLOBAL_VAR(1) struct eclite_device ccg_pd_device = {
	.name = "CCG_PD",
	.device_typ = DEV_UCSI,
	.init = ccg_pd_init,
	.driver_api = &ccg_api,
	.driver_data = &ccg_data,
	.isr = ccg_isr,
	.hw_interface = &ccg_hw_interface,
};

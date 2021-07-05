/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @file
 * @brief Sample Application for CAN (Controller Area Network).
 * This example demonstrates how to use CAN bus driver to transmit
 * and receive standard and extended messages over CAN bus for both
 * normal and FD(flexible data-rate) mode.
 * @{
 */

/**
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more details on how to
 * build the sample codes.
 */

/**
 * @brief Hardware setup.
 * Please refer section 3.4.1 in “IntelPSE_SDK_User_Guide” for more
 * details for hardware setup.
 */

/* Local Includes */
#include <drivers/can.h>
#include <sys/printk.h>
#include <zephyr.h>

/*
 * @brief can_baud_rate enum
 * List of supported baud rates of PSE CAN
 */
enum can_baud_rates_t {
	CAN_BAUDRATE_20_KBPS    = 20000U,
	CAN_BAUDRATE_50_KBPS    = 50000U,
	CAN_BAUDRATE_100_KBPS   = 100000U,
	CAN_BAUDRATE_125_KBPS   = 125000U,
	CAN_BAUDRATE_250_KBPS   = 250000U,
	CAN_BAUDRATE_500_KBPS   = 500000U,
	CAN_BAUDRATE_800_KBPS   = 800000U,
	CAN_BAUDRATE_1_MBPS     = 1000000U,
	CAN_BAUDRATE_2_MBPS     = 2000000U,
};

/*
 * @brief display data
 * Display value of all fields of the received frame
 */
static void display_can_data(uint32_t can_msg_type, struct zcan_frame *msg)
{
	int i;
	uint8_t *data;

	if (msg != NULL) {
		printk("\n-----Received Message----\n");
		printk("\nTimeStamp = %x\n", msg->timestamp);
		printk("\nID: %x", msg->can_id);
		printk("\nType: %x", msg->id_type);
		printk("\nRTR: %x", msg->rtr);
		switch (can_msg_type) {
		case CAN_STANDARD_IDENTIFIER:
		case CAN_EXTENDED_IDENTIFIER:
			printk("\nid: %x", msg->id);
			break;
		default:
			printk("\n: Unsupported id");
			break;
		}
		printk("\nDLC: %x", msg->dlc);
		if (!msg->rtr) {
			if (msg->dlc > 8) {
				data = (uint8_t *)&msg->data_32;
			} else {
				data = (uint8_t *)&msg->data;
			}

			printk("\n");
			for (i = 0; i < msg->dlc; i++) {
				printk("DATA[%d]: 0x%x ", i, data[i]);
				if ((i + 1) % 4 == 0) {
					printk("\n");
				}
			}
		} else {
			printk("\nRemote message received\n");
		}
	} else {
		printk("\nRX callback msg is NULL!!\n");
	}
}

/*
 * @brief CAN send standard messages
 *
 * This function sends STD frame with
 * CAN id as mentioned by param id.
 *
 */
void send_std_message(const struct device *dev, uint32_t id)
{
	int ret;
	/* Data payload to be sent in the frame */
	const uint8_t data[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

	ret = can_write(dev, &data[0], sizeof(data), id, CAN_DATAFRAME,
			K_FOREVER);
	if (ret < 0) {
		printk("\nCAN send failed !!\n");
	} else {
		printk("\nCAN send success !!\n");
	}
}

/*
 * @brief CAN send extended messages
 *
 * This function sends EXT frame with
 * CAN id as mentioned by param id.
 *
 */
void send_ext_message(const struct device *dev, uint32_t id)
{
	int ret;
	/* Data payload to be sent in the frame */
	const uint8_t data[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

	id |= 0xFFFF0;

	ret = can_write(dev, &data[0], sizeof(data), id, CAN_DATAFRAME,
			K_FOREVER);
	if (ret < 0) {
		printk("\nCAN EXT send failed !!\n");
	} else {
		printk("\nCAN EXT send success !!\n");
	}
}

/*
 * @brief CAN send standard FD messages
 *
 * This function sends STD FD frame with
 * CAN id as mentioned by param id.
 *
 */
void send_std_FD_message(const struct device *dev, uint32_t id)
{
	int ret;
	/* Data payload to be sent in the frame */
	const uint8_t data[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

	ret = can_write(dev, &data[0], sizeof(data), id, CAN_DATAFRAME,
			K_FOREVER);
	if (ret < 0) {
		printk("\nCAN FD send failed ret = %d !!\n", ret);
	} else {
		printk("\nCAN FD send success !!\n");
	}
}

/*
 * @brief CAN send Extended FD messages
 *
 * This function sends EXT FD frame with
 * CAN id as mentioned by param id.
 *
 */
void send_ext_FD_message(const struct device *dev, uint32_t id)
{
	int ret;
	const uint8_t data[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

	id |= 0xFFFF0;
	ret = can_write(dev, &data[0], sizeof(data), id, CAN_DATAFRAME,
			K_FOREVER);
	if (ret < 0) {
		printk("\nCAN FD EXT send failed !!\n");
	} else {
		printk("\nCAN FD EXT send success !!\n");
	}
}

/*
 * @brief CAN callback for standard frames
 *
 * This callback would be called on the
 * reception of CAN standard frames
 *
 */
void rx_std_callback(struct zcan_frame *msg, void *param)
{
	printk("\n******RX STD callback ******\n");
	/* If correctly received, the msg parameter shall contain
	 * the fields of the received CAN standard frame.
	 */
	display_can_data(CAN_STANDARD_IDENTIFIER, msg);
	printk("\nEnd of RX STD CALLABCK\n");
}

/*
 * @brief CAN callback for extended frames
 *
 * This callback would be called on the
 * reception of extended frames
 *
 */
void rx_ext_callback(struct zcan_frame *msg, void *param)
{
	/* If correctly received, the msg parameter shall contain
	 * the fields of the received CAN extended frame.
	 */
	printk("\n******RX EXT callback ******\n");
	display_can_data(CAN_EXTENDED_IDENTIFIER, msg);
	printk("\nEnd of RX EXT CALLABCK\n");
}

/* Store the successfully attached filter ids in the below array
 * Later it will be used to detach the filters.
 */
uint8_t filters[CONFIG_CAN0_STD_FILTER_COUNT + CONFIG_CAN0_EXT_FILTER_COUNT];
uint8_t num_filters;

/*
 * @brief configure filter for standard frames
 *
 * This function configures a filter for standard frames
 * with filter id as mentioned by param id.
 *
 */
void configure_std_filter(const struct device *dev, uint32_t id)
{
	struct zcan_filter filter;
	int filter_id;

	/* populate the CAN filter
	 * Filter set to receive standard can frame
	 */
	filter.id = id;
	filter.id_type = CAN_STANDARD_IDENTIFIER;
	filter.rtr = CAN_DATAFRAME;
	filter.id_mask = 0x1;

	/* Whenever the filter matches, the callback function is called */
	filter_id = can_attach_isr(dev, &rx_std_callback, NULL, &filter);
	if (filter_id < 0) {
		printk("Configuring Std filter failed !!\n");
	} else {
		printk("Configuring Std filter id %x Success ret = %d!!\n", id,
		       filter_id);
		filters[num_filters++] = filter_id;
	}
}

/*
 * @brief configure filter for extended frames
 *
 * This function configures a filter for extended frames
 * with filter id as mentioned by param id.
 *
 */
void configure_ext_filter(const struct device *dev, uint32_t id)
{
	struct zcan_filter filter;
	int filter_id;

	/* populate the CAN filter
	 * Filter set to receive extended can frame
	 */

	id |= 0xFFFF0;
	filter.id = id;
	filter.id_type = CAN_EXTENDED_IDENTIFIER;
	filter.rtr = CAN_DATAFRAME;
	filter.id_mask = id;

	filter_id = can_attach_isr(dev, &rx_ext_callback, NULL, &filter);
	if (filter_id < 0) {
		printk("Configuring Ext filter failed !!\n");
	} else {
		printk("Configuring Ext filter id %x Success ret = %d!!\n", id,
		       filter_id);
		filters[num_filters++] = filter_id;
	}
}

void main(void)
{
	const struct device *can_dev[2] = { NULL, NULL };
	int can_id = 1, i = 0;

	printk("PSE CAN Test Application\n");
	can_dev[0] = device_get_binding("CAN_SEDI_0");
	can_dev[1] = device_get_binding("CAN_SEDI_1");

	if (!can_dev[0] || !can_dev[1]) {
		printk("CAN: Device driver not found\n");
		return;
	}

	printk("\nCAN set NORMAL mode\n");
	can_set_mode(can_dev[1], CAN_NORMAL_MODE);
	can_set_mode(can_dev[0], CAN_NORMAL_MODE);

	printk("\nCAN set bit rate\n");
	can_set_bitrate(can_dev[0], CAN_BAUDRATE_500_KBPS, CAN_BAUDRATE_500_KBPS);
	can_set_bitrate(can_dev[1], CAN_BAUDRATE_500_KBPS, CAN_BAUDRATE_500_KBPS);

	printk("\nConfigure filters for STD CAN frames\n");
	configure_std_filter(can_dev[1], can_id);
	printk("\nTransmit STD CAN frames on CAN 0\n");
	send_std_message(can_dev[0], can_id);

	printk("\nConfigure filters for Extended CAN frames\n");
	configure_ext_filter(can_dev[1], can_id);
	printk("\nTransmit EXT CAN frames on CAN 1\n");
	send_ext_message(can_dev[0], can_id);

#if defined(CONFIG_CAN_FD_MODE)
	printk("\nCAN set FD mode\n");
	can_set_mode(can_dev[0], CAN_FD_MODE);
	can_set_mode(can_dev[1], CAN_FD_MODE);

	printk("\nCAN set bitrate\n");

	can_set_bitrate(can_dev[0], CAN_BAUDRATE_1_MBPS, CAN_BAUDRATE_1_MBPS);
	can_set_bitrate(can_dev[1], CAN_BAUDRATE_1_MBPS, CAN_BAUDRATE_1_MBPS);

	printk("\nConfigure Std FD filter\n");
	configure_std_filter(can_dev[1], can_id);

	printk("\nSend Std FD message\n");
	send_std_FD_message(can_dev[0], can_id);

	printk("\nConfigure FD Ext filter\n");
	configure_ext_filter(can_dev[1], can_id);
	printk("\nSend Ext FD msg\n");
	send_ext_FD_message(can_dev[0], can_id);
#endif
	printk("\nPerform cleanup\n");
	printk("\nDetaching %d Filters\n", num_filters);
	/* Clean up and remove the filters from the CAN interfaces */
	for (i = 0; i < num_filters; i++) {
		can_detach(can_dev[0], filters[i]);
		can_detach(can_dev[1], filters[i]);
	}
	printk("Exiting CAN App.\n");
}

/**
 * @}
 */

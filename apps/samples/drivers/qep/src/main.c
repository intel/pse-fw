/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @file
 * @brief Sample Application for QEP (Quadrature Encoder Pulse).
 * The sample app is for QEP edge capture and decoder.
 * The encoder used to test QEP is Rotary Quadrature Encoder
 * 256PPR/1024 CPR with Index. More details on QEP
 * encoder used can be found by visiting the link below:
 * https://www.bourns.com/docs/Product-Datasheets/EMS22Q.pdf
 * This encoders Phase A, Phase B and index is connected to the
 * QEP_2 interface.
 * @{
 */

/**
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more details on how to
 * build the sample codes.
 */

/**
 * @brief Hardware setup.
 * Please refer section 3.10.1 in “IntelPSE_SDK_User_Guide” for more details
 * for hardware setup.
 *
 * Note: Hardware connectivity differs when using Intel Add-In-Card(AIC).
 * Please refer to the relevant sections for QEP hardware connectivity
 * and instance selection in AIC user manual when using Intel Add-In-Card.
 */

/* Local Includes */
#include <sys/printk.h>
#include <drivers/qep.h>
#include <string.h>
#include <drivers/uart.h>
#include <zephyr.h>

/** QEP device name. PSE supports four instances,
 * namely QEP_0, QEP_1, QEP_2 and QEP_3.
 */
#define QEP_TEST_DEV "QEP_2"

/** Pulse per rev in quadrature decoder mode */
#define ENCODER_PULSES_PER_REV (256)

/** Edge Capture Length */
#define QEP_EDGE_CAP_LEN (5)

/** Poll count for decode mode */
#define QEP_POLL_COUNT (100)

/** Sleep Delay in ms after each position count and direction is received */
#define QEP_POLL_DELAY_MS (500)

/**  Noise filter width in nanoseconds */
#define QEP_NOISE_FILTER_WIDTH_NS (500)

/** Watchdog timeout in microseconds, to detect stalls when operating in
 * quadrature decoder mode.
 */
#define QEP_WDT_TIMEOUT_US (500)

/** Flag to enable or disable watchdog timeouts when in QEP decode mode */
#define QEP_WDT_EN_FLAG (true)

/** Flag to enable or disable filter */
#define QEP_FILTER_EN_FLAG (true)

/** Select swapping of inputs Phase A and Phase B */
#define QEP_SWAP_A_B_INPUT (false)

/** Define semaphore for edge capture */
static K_SEM_DEFINE(cap_done_sem, 0, 1);

/** Capture buffer to store the timestamp values */
static uint64_t cap_buff[QEP_EDGE_CAP_LEN];

/*
 * @brief QEP display direction of movement.
 *
 * This function displays the reported position change,
 * that is, the direction of movement.
 *
 */
static void movement_direction(uint32_t dir)
{
	switch (dir) {
	case QEP_DIRECTION_CLOCKWISE:
		printk("Clockwise\n");
		break;
	case QEP_DIRECTION_COUNTER_CLOCKWISE:
		printk("Counter Clockwise\n");
		break;
	case QEP_DIRECTION_UNKNOWN:
		printk("Direction Unknown\n");
		break;
	default:
		printk("Invalid Direction\n");
	}
}

/*
 * @brief QEP notify event reported.
 *
 * This function is used to display the events
 * reported, such as, counter reset, direction change,
 * watchdog timeout, phase error and edge cap completion.
 *
 */
static void notify_event(qep_event_t event)
{
	switch (event) {
	case QEP_EVENT_WDT_TIMEOUT:
		printk("Watchdog timeout.\n");
		break;
	case QEP_EVENT_CTR_RST_UP:
		printk("Position counter reset to counting up.\n");
		break;
	case QEP_EVENT_CTR_RST_DN:
		printk("Position counter reset to counting down.\n");
		break;
	case QEP_EVENT_DIR_CHANGE:
		printk("Direction change.\n");
		break;
	case QEP_EVENT_PHASE_ERR:
		printk("Phase error detected.\n");
		break;
	case QEP_EVENT_EDGE_CAP_DONE:
		printk("Capture done for given count.\n");
		break;
	case QEP_EVENT_EDGE_CAP_CANCELLED:
		printk("Capture cancelled.\n");
		break;
	case QEP_EVENT_UNKNOWN:
	default:
		printk("Unknown event detected.\n");
	}
}

/*
 * @brief QEP callback for event of edge capture.
 *
 * This callback would be called on the event of
 * edge capture. By default the capture length
 * is set to 5.
 *
 */
static void qep_callback(const struct device *dev, void *param, qep_event_t event,
			 uint32_t len)
{
	notify_event(event);

	if (event == QEP_EVENT_EDGE_CAP_DONE) {
		printk("Edge capture done , len =%d\n", len);
		k_sem_give(&cap_done_sem);
	}
}

/*
 * @brief QEP test edge capture
 *
 * This capture function is independent of quadrature decoding.
 * This supports capturing 64-bit timestamps during transitions
 * on Phase A input.
 *
 */
static int qep_edge_capture(const struct device *dev)
{
	int ret;
	struct qep_config zeph_qep_config = { 0 };

	printk("Testing Edge Capture Mode\n");
	printk("Waiting for %d Edges\n", QEP_EDGE_CAP_LEN);

	/* Configure the QEP device to set the mode to edge capture,
	 * swapping of inputs phase A and Phase B, edge type to be
	 * used as trigger, noise filter width in nanoseconds with
	 * flag enabled/disabled, and whether to capture event on
	 * both rising and falling edge.
	 */
	zeph_qep_config.mode = QEP_MODE_EDGE_CAPTURE;
	zeph_qep_config.swap_a_b_input = QEP_SWAP_A_B_INPUT;
	zeph_qep_config.edge_type = QEP_EDGE_SELECT_RISING;
	zeph_qep_config.filter_width_ns = QEP_NOISE_FILTER_WIDTH_NS;
	zeph_qep_config.filter_en = QEP_FILTER_EN_FLAG;
	zeph_qep_config.cap_edges = QEP_EDGE_CAP_DOUBLE;
	/* Configure operational parameters for QEP */
	ret = qep_config_device(dev, &zeph_qep_config);
	if (ret != 0) {
		printk("QEP config failed ec =%d\n", ret);
		return -EIO;
	}

	/* Start QEP Edge capture */
	ret = qep_start_capture(dev, cap_buff, QEP_EDGE_CAP_LEN,
				(qep_callback_t) qep_callback, NULL);
	if (ret != 0) {
		printk("Capture Failed %d\n", ret);
		return -EIO;
	}
	k_sem_take(&cap_done_sem, K_FOREVER);
	for (int i = 0; i < QEP_EDGE_CAP_LEN; i++) {
		printk("Timestamp %d: %llu\n", i, cap_buff[i]);
	}
	printk("Passed\n");

	return 0;
}

/*
 * @brief QEP decoder
 *
 * This function provides current position counter value and the
 * direction of rotation in QEP mode.
 *
 */
static int qep_decoder(const struct device *dev)
{
	int ret;
	int pos_count;
	int direction;
	struct qep_config zeph_qep_config = { 0 };

	printk("Testing QEP Decode Mode\n");

	/* Configure the QEP device to set the mode to decoder mode,
	 * swapping of inputs phase A and Phase B, reset position
	 * count after reaching the programmed max count, pulse per
	 * rev in quadrature decoder mode, select rising edge for events,
	 * noise filter width in nanoseconds with flag enabled/disabled,
	 * watchdog timeout in microseconds, to detect stalls with flag
	 * enabled/disabled.
	 */
	zeph_qep_config.mode = QEP_MODE_QEP_DECODER;
	zeph_qep_config.swap_a_b_input = QEP_SWAP_A_B_INPUT;
	zeph_qep_config.pos_ctr_rst = QEP_POS_CTR_RST_ON_MAX_CNT;
	zeph_qep_config.pulses_per_rev = ENCODER_PULSES_PER_REV;
	zeph_qep_config.edge_type = QEP_EDGE_SELECT_RISING;
	zeph_qep_config.filter_width_ns = QEP_NOISE_FILTER_WIDTH_NS;
	zeph_qep_config.filter_en = QEP_FILTER_EN_FLAG;
	zeph_qep_config.wdt_timeout_us = QEP_WDT_TIMEOUT_US;
	zeph_qep_config.wdt_en = QEP_WDT_EN_FLAG;
	/* Valid only when QEP_POS_CTR_RST_ON_IDX_EVT is selected. */
	zeph_qep_config.index_gating = QEP_PH_A_HIGH_PH_B_HIGH;
	/* Configure operational parameters for QEP */
	ret = qep_config_device(dev, &zeph_qep_config);
	if (ret != 0) {
		printk("Qep config failed ec =%d\n", ret);
		return -EIO;
	}

	/* Disables Watchdog timeout event in QEP decode mode */
	ret = qep_disable_event(dev, QEP_EVENT_WDT_TIMEOUT);
	if (ret != 0) {
		printk("Disable event failed ec =%d\n", ret);
	} else {
		printk("Disabled ");
		notify_event(QEP_EVENT_WDT_TIMEOUT);
	}

	/* Starts the QEP decode operation, events are notified
	 * through callback.
	 */
	ret = qep_start_decode(dev, (qep_callback_t) qep_callback, NULL);
	if (ret != 0) {
		printk("Start qep decode failed ec = %d\n", ret);
		return -EIO;
	}

	/* Disables direction change event in QEP decode mode */
	ret = qep_disable_event(dev, QEP_EVENT_DIR_CHANGE);
	if (ret != 0) {
		printk("Disable event failed ec =%d\n", ret);
	} else {
		printk("Disabled ");
		notify_event(QEP_EVENT_DIR_CHANGE);
	}

	for (int i = 0; i < QEP_POLL_COUNT; i++) {

		/* qep_get_position_count provides current position counter
		 * value.
		 * The position count is updated using the pulses per revolution
		 * field provided during the device configuration.
		 */
		ret = qep_get_position_count(dev, &pos_count);
		if (ret == 0) {
			printk("Position Count  = %d\n", pos_count);
		} else {
			printk("Pos Error %d\n", ret);
			return -EIO;
		}

		/* qep_get_direction provides direction of rotation in QEP mode
		 * based on the last position counter reset event.
		 * The direction of rotation is clockwise, counter clockwise
		 * or unknown if no counter reset event has occurred.
		 */
		ret = qep_get_direction(dev, &direction);
		if (ret == 0) {
			movement_direction(direction);
		} else {
			printk("Direction Error %d\n", ret);
			return -EIO;
		}
		k_sleep(K_MSEC(QEP_POLL_DELAY_MS));
	}

	/* Stop an ongoing QEP decode operation. */
	ret = qep_stop_decode(dev);
	if (ret != 0) {
		printk("Stop decode Error %d\n", ret);
		return -EIO;
	}
	printk("Decode stopped\n");
	return 0;
}

/*
 * @brief main function
 *
 * QEP driver in edge capture mode and decode mode to get the current
 * position counter value and the last known direction in decode mode.
 *
 */
void main(void)
{
	/* Get the device handle for QEP device by QEP_TEST_DEV. */
	const struct device *dev = device_get_binding(QEP_TEST_DEV);

	if (dev == NULL) {
		printk("Failed to get QEP binding\n");
		return;
	}

	qep_edge_capture(dev);
	qep_decoder(dev);

	while (true) {
		k_sleep(K_MSEC(1000));
	}
}

/**
 * @}
 */

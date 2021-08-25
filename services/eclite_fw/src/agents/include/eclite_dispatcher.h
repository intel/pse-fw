/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file eclite_dispatcher.h
 *
 * @brief eclite dispatcher for handling various events,
 */

#ifndef __ECLITE_DISPATCHER_H__
#define __ECLITE_DISPATCHER_H__

#include <stdint.h>
#include <logging/log.h>
#include <zephyr.h>
/** @brief eclite events.
 *
 * eclite event codes.
 */
enum eclite_events {
	/** HECI Driver Event */
	HECI_EVENT,
	/** Periodic Timer Event */
	TIMER_EVENT,
	/** GPIO Event */
	GPIO_EVENT,
	/** Charger Event */
	CHG_EVENT,
	/** Fuelgauge Event */
	FG_EVENT,
	/** Thermal Event */
	THERMAL_EVENT,
	/** UCSI Event */
	UCSI_EVENT,
};

/** @brief eclite queue data table.
 *
 * event queue data structure for posting event data to dispatcher.
 */
struct dispatcher_queue_data {
	/** Dispatcher event */
	enum eclite_events event_type;
	/** Data for dispatcher */
	uint32_t data;
};

/**
 * @brief This routine insert event for EClite dispatcher processing.
 *
 * @note Can be called by ISRs.
 *
 * @param event_data data for event.
 *
 * @retval 0 Event inserted successfully.
 * @retval -ENOMSG Returned without waiting or queue purged.
 * @retval -EAGAIN Waiting period timed out.
 */
int eclite_post_dispatcher_event(struct dispatcher_queue_data *event_data);

/**
 * @brief This routine initialize and starts dispatcher services.
 *
 * @retval Fail initialization failed, dispatcher services not started.
 * @retval SUCCESS initialization successful.
 */
int dispatcher_init(void);

/**
 * @brief @brief Kernel objects used by dispatcher.
 *
 */
extern struct k_thread dispatcher_task;
extern struct k_msgq dispatcher_queue;
extern struct k_timer dispatcher_timer;

/**
 * @brief Timer callback functions
 *
 */
extern void timer_periodic_callback(struct k_timer *timer);
extern void timer_stop_callback(struct k_timer *timer);
extern int eclite_sx_state;

#endif /*_ECLITE_DISPATCHER_H_ */

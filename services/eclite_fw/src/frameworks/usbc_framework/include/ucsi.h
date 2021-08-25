/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief This file exposes interface for the UCSI interface.
 */

#include <stdint.h>
#include <string.h>
#include <logging/log.h>
#include <common.h>

#define MAX_UCSI_CMD_N 21
#define UCSI_MSG_SIZE 16
#define UCSI_CMD_DATA 6

/**
 * @brief This function define USCI function prototypes.
 *
 * @param dev is eclite device pointer.
 *
 * retval same as ucsi_framework.
 */
typedef int (*usbc_fn_t)(void *dev);
typedef void (*pd_state_t)(uint8_t port, uint8_t cmd, uint8_t len,
			   uint8_t *buf);

/**
 * @cond INTERNAL_HIDDEN
 *
 * This define are considerring future need of device driver
 * if we need to support different PD vendors below cmd table used
 */

enum ucsi_commad {
	RESERVED = 0,
	PPM_RSTCMD,
	PPM_CANCELCMD,
	PPM_CONNRSTCMD,
	PPM_ACKCCCICMD,
	PPM_SET_NOTENCMD,
	PPM_GET_CAPCMD,
	PPM_GET_CONNCAPCMD,
	PPM_SET_USBOPMODECMD,
	PPM_SET_USBOPROLECMD,
	PPM_SET_PWRDIRMODECMD,
	PPM_SET_PWRDIRROLECMD,
	PPM_GET_ALTMODES,
	PPM_GET_CAMSUPPCMD,
	PPM_GET_CURRCAMCMD,
	PPM_SET_NEWCAMCMD,
	PPM_GET_PDOSCMD,
	PPM_GET_CABLEPROPCMD,
	PPM_GET_CONNSTATCMD,
	PPM_GET_ERRSTATUS,
	PPM_SET_PWRLIMIT,
};

enum usbc_buf {
	USBC_MSG_IN = 0,
	USBC_MSG_OUT,
};

struct msg_in_t {
	uint8_t msg[UCSI_MSG_SIZE];
};

struct msg_out_t {
	uint8_t msg[UCSI_MSG_SIZE];
};

struct control_t {
	uint8_t command;
	uint8_t dataLen;
	uint8_t cmdspecific[UCSI_CMD_DATA];
};

struct cci_t {
	uint8_t reserved0 : 1;
	uint8_t cci : 7;
	uint8_t dataLen;
	uint8_t reserved1;
	uint8_t reserved2 : 1;
	uint8_t notsupported : 1;
	uint8_t cancelcompleted : 1;
	uint8_t resetcompleted : 1;
	uint8_t busy : 1;
	uint8_t ackcmdcomp : 1;
	uint8_t error : 1;
	uint8_t cmdcompleted : 1;
};

/**
 * @endcond
 */

/**
 * @brief UCSI framework data structure.
 * Structure contains parameter for the UCSI operation as mentioned in the UCSI
 * Spec.
 */
struct ucsi_data_t {
	struct msg_in_t msg_in;
	struct msg_out_t msg_out;
	struct cci_t cci;
	struct control_t control;
};


/**
 * @brief UCSI  framewrok APIs.
 *
 * These APIs are used to process UCSI commands.
 */
struct ucsi_api_t {
	usbc_fn_t ucsi_fn;
	pd_state_t get_pd_state;
};

/** UCSI Version number **/
#define UCSI_VERSION_STR     0x110 /* Version 1.1 */

#ifdef CONFIG_ECLITE_UCSI_DBG
/** EClite debug macro.*/
#define ECLITE_USBC_ERR(param ...) printk("[ECLITE-ERROR]: " param)
#define ECLITE_USBC_DBG(param ...) printk("[ECLITE-DEBUG]: " param)
#define ECLITE_USBC_INFO(param ...) printk("[ECLITE-INFO]: " param)

#else
/** EClite debug macro.*/
#define ECLITE_USBC_ERR(param ...)
#define ECLITE_USBC_DBG(param ...)
#define ECLITE_USBC_INFO(param ...)

#endif /* CONFIG_ECLITE_UCSI_DBG */

/**
 * @brief This function acts as mailbox between UCSI and external world to
 * transfer data.
 *
 * @param cmd is usbc_buf enumeration type.
 * @param len is buffer to be copied.
 * @param buf is pointer to input buffer.
 *
 * @retval SUCCESS if correct comamnd is issued.
 *         FAILURE if incorrect command.
 */
int ucsi_command(uint8_t cmd, uint8_t len, uint8_t *buf);


/**
 * @brief This function acts as mailbox between UCSI and external world to
 * transfer data/from PD.
 *
 * @param cmd_id is UCSI command index.
 *
 * @retval SUCCESS if correct comamnd is issued.
 *         FAILURE if incorrect command.
 */
int ucsi_framework(uint8_t cmd_id);


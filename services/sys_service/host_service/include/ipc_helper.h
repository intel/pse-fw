/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****** IPC helper definitions *****/

#ifndef __IPC_HELPER_H
#define __IPC_HELPER_H

#include "driver/sedi_driver_ipc.h"
#include "device.h"

#define MNG_CAP_SUPPORTED  1

#define IPC_PROTOCOL_BOOT 0
#define IPC_PROTOCOL_HECI 1
#define IPC_PROTOCOL_MCTP 2
#define IPC_PROTOCOL_MNG 3
#define IPC_PROTOCOL_INVAILD 0xF

#define IPC_DATA_LEN_MAX (128)

#define IPC_HEADER_LENGTH_MASK (0x03FF)
#define IPC_HEADER_PROTOCOL_MASK (0x0F)
#define IPC_HEADER_MNG_CMD_MASK (0x0F)

#define IPC_HEADER_LENGTH_OFFSET 0
#define IPC_HEADER_PROTOCOL_OFFSET 10
#define IPC_HEADER_MNG_CMD_OFFSET 16
#define IPC_DRBL_BUSY_OFFS 31

#define IPC_HEADER_GET_LENGTH(drbl_reg)	\
	(((drbl_reg) >> IPC_HEADER_LENGTH_OFFSET) & IPC_HEADER_LENGTH_MASK)
#define IPC_HEADER_GET_PROTOCOL(drbl_reg) \
	(((drbl_reg) >> IPC_HEADER_PROTOCOL_OFFSET) & IPC_HEADER_PROTOCOL_MASK)
#define IPC_HEADER_GET_MNG_CMD(drbl_reg) \
	(((drbl_reg) >> IPC_HEADER_MNG_CMD_OFFSET) & IPC_HEADER_MNG_CMD_MASK)

#define IPC_BUILD_DRBL(length, protocol)	      \
	((1 << IPC_DRBL_BUSY_OFFS)		      \
	 | ((protocol) << IPC_HEADER_PROTOCOL_OFFSET) \
	 | ((length) << IPC_HEADER_LENGTH_OFFSET))

#define IPC_BUILD_MNG_DRBL(cmd, length)			      \
	(((1) << IPC_DRBL_BUSY_OFFS)			      \
	 | ((IPC_PROTOCOL_MNG) << IPC_HEADER_PROTOCOL_OFFSET) \
	 | ((cmd) << IPC_HEADER_MNG_CMD_OFFSET)		      \
	 | ((length) << IPC_HEADER_LENGTH_OFFSET))

#ifdef CONFIG_SYS_MNG
int send_rx_complete(const struct device *dev);
#define RTD3_NOTIFIED_STUCK     ((int32_t)-1)
#define RTD3_SWITCH_TO_SX	((int32_t)-2)

/*!
 * \fn int mng_host_access_req(s32_t timeout)
 * \brief request access to host
 * \param[in] timeout: timeout time (in milliseconds)
 * \return 0 or error codes
 *
 * @note mng_host_access_req and mng_host_access_dereq should be called in pair
 */
extern int (*mng_host_access_req)(int32_t timeout);

void mng_host_access_dereq(void);
void mng_sx_entry(void);

#endif

#if CONFIG_HECI
void heci_reset(void);
#endif

#endif  /* __IPC_HELPER_H */

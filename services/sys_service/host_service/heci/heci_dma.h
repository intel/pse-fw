/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "heci_internal.h"

#define GET_NUM_PAGE_BITMAPS(size) \
	((size + BITS_PER_DW - 1) / BITS_PER_DW)
#define GET_NUM_PAGES(size) \
	((size + CONFIG_HECI_PAGE_SIZE - 1) / CONFIG_HECI_PAGE_SIZE)
#define BITMAP_SLC(idx) ((idx) / BITS_PER_DW)
#define BITMAP_BIT(idx) ((idx) % BITS_PER_DW)

#define GET_MSB(data64) ((uint32_t)(data64 >> 32))
#define GET_LSB(data64) ((uint32_t)(data64))

#define HECI_DMA_DEV_NAME "DMA_0"
#define HECI_DMA_DEV SEDI_DMA_0
#define HECI_DMA_DEV_CHN 1
#define DMA_TIMEOUT_MS 500

extern heci_device_t heci_dev;
extern struct k_mutex dev_lock;

bool send_client_msg_dma(heci_conn_t *conn, mrd_t *msg);

void heci_dma_alloc_notification(heci_bus_msg_t *msg);

void heci_dma_xfer_ack(heci_bus_msg_t *msg);

/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "heci_internal.h"
#include "heci_dma.h"
#include "ipc_helper.h"
#include "sedi.h"
#include "string.h"
#include "host_ipc_service.h"
#include <user_app_framework/user_app_framework.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(heci, CONFIG_HECI_LOG_LEVEL);

/*
 * when host enables HECI via DMA, it will allocate buffer in DDR and send the
 * buffer info to fw, fw controls the fw, and split it into pages in logic. When
 * sending msg to host, enough pages is needed. After DMA transferring, will
 * free the pages used
 */
void heci_dma_alloc_notification(heci_bus_msg_t *msg)
{
	int status = 1;
	heci_bus_dma_alloc_notif_req_t *req =
		(heci_bus_dma_alloc_notif_req_t *)msg->payload;
	heci_bus_dma_alloc_resp_t resp = { 0 };
	struct host_rx_dma_info *dma_info = &heci_dev.host_rx_dma;

	/* need at least one valid buffer from host */
	if (msg->hdr.len < sizeof(*req) + sizeof(dma_buf_info_t)) {
		LOG_ERR("no valid dma buf for heci");
		goto out;
	}

	memset(dma_info, 0, sizeof(*dma_info));
	dma_info->dma_addr = req->alloc_dma_buf[0].buf_address;
	dma_info->size = req->alloc_dma_buf[0].buf_size;
	/* ddr size and addr must be page aligned*/
	if ((dma_info->size % CONFIG_HECI_PAGE_SIZE)
	    || (dma_info->dma_addr % CONFIG_HECI_PAGE_SIZE)) {
		LOG_ERR("illegal host addr allocated for HECI");
		goto out;
	}

	LOG_DBG("DDR alloced for HECI: 0x%x%08x+%08x",
		GET_MSB(dma_info->dma_addr),
		GET_LSB(dma_info->dma_addr),
		dma_info->size);
	if (dma_info->size > MAX_HOST_SIZE) {
		dma_info->size = MAX_HOST_SIZE;
	}
	k_sem_init(&dma_info->dma_sem, 0, UINT_MAX);
	k_mutex_init(&dma_info->dma_lock);
	dma_info->num_pages = GET_NUM_PAGE_BITMAPS(dma_info->size);
	status = 0;

	/*response host*/
out:
	resp.command = HECI_BUS_MSG_DMA_ALLOC_RESP;
	resp.status = status;

	heci_send_proto_msg(HECI_DRIVER_ADDRESS, HECI_DRIVER_ADDRESS,
			    true, (uint8_t *)&resp, sizeof(resp));
}

/* get enough buffer in host for the sending*/
static bool heci_dma_get_tx_buffer(uint64_t *dma_addr, uint32_t req_len)
{
	int i, j, pages;
	bool found;
	struct host_rx_dma_info *dma_info = &heci_dev.host_rx_dma;

	if (dma_info->num_pages == 0) {
		LOG_ERR("ddr is not ready");
		return false;
	}
	pages = GET_NUM_PAGES(req_len);
	if (pages > PAGE_BITMAP_NUM * BITS_PER_DW) {
		LOG_ERR("too large buf to get");
		return false;
	}
	for (i = 0; i <= dma_info->num_pages - pages; i++) {
		/* look for fist available page*/
		if (atomic_test_bit(
			    &dma_info->page_bitmap[BITMAP_SLC(i)],
			    BITMAP_BIT(i))) {
			continue;
		}

		found = true;
		/* check if the next few ones are available*/
		for (j = i; j < i + pages; j++) {
			if (atomic_test_bit(
				    &dma_info->page_bitmap[BITMAP_SLC(j)],
				    BITMAP_BIT(j))) {
				found = false;
				break;
			}
		}
		/* if not found, continue to look for */
		if (!found) {
			i = j;
			continue;
		}

		/* if found, mark the found pages as used, and return address*/
		for (j = i; j < i + pages; j++) {
			atomic_set_bit(
				&dma_info->page_bitmap[BITMAP_SLC(j)],
				BITMAP_BIT(j));
		}
		*dma_addr = dma_info->dma_addr +
			    i * CONFIG_HECI_PAGE_SIZE;
		LOG_DBG("0x%x%08x+%u", GET_MSB(*dma_addr),
			GET_LSB(*dma_addr), req_len);
		return true;
	}
	LOG_WRN("fail for no free buffer");
	return false;
}

/* free allocated buffer for DMA when host gets the msg*/
static void heci_dma_free_tx_buffer(uint64_t dma_addr, uint32_t len)
{
	uint32_t pages, page_index;
	struct host_rx_dma_info *dma_info = &heci_dev.host_rx_dma;

	if (dma_info->num_pages == 0) {
		LOG_ERR("ddr is not ready");
	}

	page_index = (dma_addr - dma_info->dma_addr) /
		     CONFIG_HECI_PAGE_SIZE;
	pages = GET_NUM_PAGES(len);
	for (int i = page_index; i < page_index + pages; i++) {
		atomic_clear_bit(&dma_info->page_bitmap[BITMAP_SLC(i)],
				 BITMAP_BIT(i));
	}
	LOG_DBG("0x%x%08x+%u",
		GET_MSB(dma_addr),
		GET_LSB(dma_addr),
		len);
}

static void dma_drvevent(sedi_dma_t device, int dma_channel_id,
			 int event_local, void *param)
{
	struct host_rx_dma_info *dma_info = &heci_dev.host_rx_dma;

	if (&dma_info->dma_sem) {
		k_sem_give(&dma_info->dma_sem);
	}
}

static int dma_copy(uint64_t src_addr, uint64_t dst_addr,
		    uint32_t len, bool is_ddr2sram)
{
	sedi_dma_t dma = HECI_DMA_DEV;
	uint8_t chn = HECI_DMA_DEV_CHN;
	int ret = 0;

	struct host_rx_dma_info *dma_info = &heci_dev.host_rx_dma;

	/*
	 * unlock the heci context so that other heci msg can continue while DMA
	 * is transferring
	 */
	k_mutex_unlock(&dev_lock);
	k_mutex_lock(&dma_info->dma_lock, K_FOREVER);

	sedi_dma_init(dma, chn, dma_drvevent, NULL);
	sedi_dma_set_power(dma, chn, SEDI_POWER_FULL);
	sedi_dma_control(dma, chn, SEDI_CONFIG_DMA_DIRECTION,
			 DMA_MEMORY_TO_MEMORY);
	sedi_dma_control(dma, chn, SEDI_CONFIG_DMA_SR_MEM_TYPE,
			 is_ddr2sram ? DMA_DRAM_MEM : DMA_SRAM_MEM);
	sedi_dma_control(dma, chn, SEDI_CONFIG_DMA_DT_MEM_TYPE,
			 is_ddr2sram ? DMA_SRAM_MEM : DMA_DRAM_MEM);

	/*
	 * when sending local msg to host, the buffer cache needs flush to
	 * memory
	 */
	if (!is_ddr2sram) {
		sedi_core_clean_dcache_by_addr((uint32_t *)GET_LSB(src_addr),
					       len + ((uint32_t)src_addr &
						      (CONFIG_DMA_ALIGN_SIZE -
						       1)));
	}

	sedi_dma_control(dma, chn, SEDI_CONFIG_DMA_BURST_LENGTH,
			 DMA_BURST_TRANS_LENGTH_1);
	sedi_dma_control(dma, chn, SEDI_CONFIG_DMA_SR_TRANS_WIDTH,
			 DMA_TRANS_WIDTH_8);
	sedi_dma_control(dma, chn, SEDI_CONFIG_DMA_DT_TRANS_WIDTH,
			 DMA_TRANS_WIDTH_8);
	sedi_dma_start_transfer(dma, chn, src_addr, dst_addr, len);

	/* release heci lock, let other client run*/
	ret = k_sem_take(&dma_info->dma_sem, K_MSEC(DMA_TIMEOUT_MS));
	if (ret) {
		sedi_dma_abort_transfer(dma, chn);
		goto out;
	}

	if (is_ddr2sram) {
		sedi_core_inv_dcache_by_addr((uint32_t *)GET_LSB(dst_addr),
					     len + ((uint32_t)dst_addr &
						    (CONFIG_DMA_ALIGN_SIZE -
						     1)));
	}

out:
	sedi_dma_set_power(dma, chn, SEDI_POWER_LOW);
	k_mutex_unlock(&dma_info->dma_lock);
	k_mutex_lock(&dev_lock, K_FOREVER);
	return ret;
}

/*
 * this message will be received when host handled the DMA msg, so will free the
 * buffer
 */
void heci_dma_xfer_ack(heci_bus_msg_t *msg)
{
	dma_msg_info_t *dmabuf;
	heci_bus_dma_xfer_resp_t *resp;

	resp = (heci_bus_dma_xfer_resp_t *)msg->payload;
	if (msg->hdr.len < sizeof(heci_bus_dma_xfer_resp_t)) {
		LOG_ERR("wrong DMA_XFER_RESP len %d\n", msg->hdr.len);
		return;
	}

	for (dmabuf = resp->dma_buf;
	     dmabuf < (dma_msg_info_t *)(msg->payload + msg->hdr.len);
	     dmabuf++) {
		/* for every dma address in xfer_ack packet that was recieved,
		 * mark the DMA pages as free and usable in internal
		 */
		heci_dma_free_tx_buffer(dmabuf->msg_addr_in_host,
					dmabuf->msg_length);
	}
}

bool send_client_msg_dma(heci_conn_t *conn, mrd_t *msg)
{
	__ASSERT(conn != NULL, "invalid heci connection\n");
	__ASSERT(msg != NULL, "invalid heci client msg to send\n");

	int ret, host_req_handler;
	uint32_t block_size;
	uint64_t dram_addr;
	heci_bus_dma_xfer_req_t req = { 0 };

	req.command = HECI_BUS_MSG_DMA_XFER_REQ;
	req.host_addr = conn->host_addr;
	req.fw_addr = conn->fw_addr;
	while (msg != NULL) {
		/* allocate enough buffer for DMA*/
		block_size = msg->len;
		ret = heci_dma_get_tx_buffer(&dram_addr, block_size);
		if (ret == false) {
			LOG_ERR("get HECI TX buffer failed");
			return ret;
		}

#ifdef CONFIG_SYS_MNG
		host_req_handler = host_access_req(HECI_HAL_DEFAULT_TIMEOUT);
		if (host_req_handler) {
			heci_dma_free_tx_buffer(dram_addr, block_size);
			return false;
		}
#endif

		/* copy msg content to host dram via DMA */
		ret = dma_copy((uint32_t) msg->buf,
			       dram_addr, msg->len, false);
#ifdef CONFIG_SYS_MNG
		host_access_dereq(host_req_handler);
#endif
		if (ret) {
			LOG_ERR("dma copy err");
			goto err_sending;
		}

		LOG_DBG("%p->0x%x%08x+%u", msg->buf,
			GET_MSB(dram_addr),
			GET_LSB(dram_addr),
			msg->len);
		/* send ipc notification to host */
		req.msg_addr_in_host = dram_addr;
		req.msg_length = msg->len;
		if (!heci_send_proto_msg(HECI_DRIVER_ADDRESS,
					 HECI_DRIVER_ADDRESS,
					 true, (uint8_t *)&req, sizeof(req))) {
			LOG_ERR("write HECI client msg err");
			goto err_sending;
		}
		msg = msg->next;
	}

	return true;

err_sending:
	heci_dma_free_tx_buffer(dram_addr, block_size);
	return false;
}

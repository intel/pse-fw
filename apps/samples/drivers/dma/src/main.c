/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

#include <device.h>
#include <drivers/dma.h>

#include <string.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

/* size of stack area used by each thread */
#define STACKSIZE               1024
/* scheduling priority used by each thread */
#define PRIORITY                7
/* delay between greetings (in ms) */
#define SLEEPTIME               50

/* max time to be waited for dma to complete (in ms) */
#define WAITTIME                1000

/* transfer 4 dma blocks*/
#define MAX_TRANSFERS 4
#define BURST_LEN 8

/* semaphore to sync the dma event and caller*/
K_SEM_DEFINE(dma_sem, 0, 1);

#define DMA_DEVICE_NAME "DMA_0"
#define RX_BUFF_SIZE (48)

struct transfers {
	const char *source;
	char *destination;
	size_t size;
};

/* 4 tx and rx buffer */
static const char tx_data[] = "potato";
static const char tx_data2[] = "likes";
static const char tx_data3[] = "horse";
static const char tx_data4[] = "my";
static char rx_data[RX_BUFF_SIZE] = { 0 };
static char rx_data2[RX_BUFF_SIZE] = { 0 };
static char rx_data3[RX_BUFF_SIZE] = { 0 };
static char rx_data4[RX_BUFF_SIZE] = { 0 };

/* blocks the sample will transfer */
static struct transfers transfer_blocks[MAX_TRANSFERS] = {
	{
		.source = tx_data,
		.destination = rx_data,
		.size = sizeof(tx_data),
	},
	{
		.source = tx_data2,
		.destination = rx_data2,
		.size = sizeof(tx_data2),
	},
	{
		.source = tx_data3,
		.destination = rx_data3,
		.size = sizeof(tx_data3),
	},
	{
		.source = tx_data4,
		.destination = rx_data4,
		.size = sizeof(tx_data4),
	},
};

static const struct device *dma_device;

/*
 * current_block_count refers the currently how many dma block is done,
 * and total_block_count is all block number needed.
 * these 2 are only used in reload mode. In LinkedList mode, set the same value
 */
static uint32_t current_block_count, total_block_count;

/* the callback that will be called when DMA is done or meets error */
static void test_done(const struct device *dev, void *user_data,
			       uint32_t channel, int error_code)
{
	uint32_t src, dst;
	size_t size;

	current_block_count++;

	if (error_code != 0) {
		/* meet error of dma */
		printk("DMA transfer met an error = 0x%x\n", error_code);
		k_sem_give(&dma_sem);
	} else if (current_block_count < total_block_count) {
		/*
		 * it is branch for reload mode, another dma transaction can be
		 * initialized in callback
		 */
		printk("reload block %d\n", current_block_count);
		src = (uint32_t)transfer_blocks[current_block_count].source;
		dst = (uint32_t)transfer_blocks[current_block_count].destination;
		size = transfer_blocks[current_block_count].size;
		/* rerun dma with params of new block*/
		dma_reload(dma_device, channel, src, dst, size);
		dma_start(dma_device, channel);
	} else {
		/*
		 * if it is the last block or it is using LinkedList mode, mark
		 * it as done, and notify caller thread.
		 */
		printk("DMA transfer done\n");
		k_sem_give(&dma_sem);
	}
}

/* test dma multi block (using reload mode) */
static int test_task(uint32_t chan_id, uint32_t blen, uint32_t block_count)
{
	struct dma_config dma_cfg = { 0 };

	/* set block params for 1st block */
	struct dma_block_config dma_block_cfg = {
		.block_size = sizeof(tx_data),
		.source_address = (uint32_t)tx_data,
		.dest_address = (uint32_t)rx_data,
	};

	if (block_count > ARRAY_SIZE(transfer_blocks)) {
		printk("block_count %u is greater than %ld\n", block_count,
		       ARRAY_SIZE(transfer_blocks));
		return -1;
	}

	dma_device = device_get_binding(DMA_DEVICE_NAME);

	if (!dma_device) {
		printk("Cannot get dma controller\n");
		return -1;
	}

	/* prepare dma config params*/
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.source_burst_length = blen;
	dma_cfg.dest_burst_length = blen;
	dma_cfg.dma_callback = test_done;
	dma_cfg.complete_callback_en = 1;
	dma_cfg.error_callback_en = 1;
	/* only use one block, will reload another when previous one is done */
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_block_cfg;

	printk("Preparing DMA Controller: Chan_ID=%u, BURST_LEN=%u\n", chan_id,
	       blen);

	/* clean rx buffer */
	memset(rx_data, 0, sizeof(rx_data));
	memset(rx_data2, 0, sizeof(rx_data2));
	memset(rx_data3, 0, sizeof(rx_data3));
	memset(rx_data4, 0, sizeof(rx_data4));
	SCB_CleanDCache_by_Addr((uint32_t *)rx_data, RX_BUFF_SIZE * 4 + 32);

	/* config to initialize dma */
	if (dma_config(dma_device, chan_id, &dma_cfg)) {
		printk("ERROR: configuring\n");
		return -1;
	}

	printk("Starting the transfer\n");

	current_block_count = 0;
	total_block_count = block_count;

	/* start dma transaction */
	if (dma_start(dma_device, chan_id)) {
		printk("ERROR: transfer\n");
		return -1;
	}

	/* Wait a while for the dma to complete */
	if (k_sem_take(&dma_sem, K_MSEC(WAITTIME))) {
		printk("*** timed out waiting for dma to complete ***\n");
		return -1;
	}

	/* Invalidate cache of rx buffer */
	SCB_InvalidateDCache_by_Addr((uint32_t *)rx_data,
				     RX_BUFF_SIZE * 4 + 32);

	/*
	 * compare the rx and tx buffer string, to check whether the dma copy is
	 * done successfully or not.
	 * if compared as the same, it means the dma is working properly, return
	 * 0, else, return -1, indicating there is something wrong
	 */
	switch (block_count) {
	case 4:
		if (strcmp(tx_data4, rx_data4) != 0) {
			return -1;
		}
		printk("%s\n", rx_data4);

	case 3:
		if (strcmp(tx_data3, rx_data3) != 0) {
			return -1;
		}
		printk("%s\n", rx_data3);

	case 2:
		if (strcmp(tx_data2, rx_data2) != 0) {
			return -1;
		}
		printk("%s\n", rx_data2);

	case 1:
		if (strcmp(tx_data, rx_data) != 0) {
			return -1;
		}
		printk("%s\n", rx_data);
		break;

	default:
		printk("Invalid block count %d\n", block_count);
		return -1;
	}

	return 0;
}

/* test dma linked list mode */
static int test_ll_task(uint32_t chan_id, uint32_t blen, uint32_t block_count)
{
	struct dma_config dma_cfg = { 0 };

	struct dma_block_config dma_block_cfg[4] = { 0 };

	/* prepare all the block in linked list */
	for (int i = 0; i < block_count; i++) {
		dma_block_cfg[i].source_address =
			(uint32_t)transfer_blocks[i].source;
		dma_block_cfg[i].dest_address =
			(uint32_t)transfer_blocks[i].destination;
		dma_block_cfg[i].block_size = transfer_blocks[i].size;
		if (i < block_count - 1) {
			dma_block_cfg[i].next_block = &(dma_block_cfg[i + 1]);
		}
	}

	dma_device = device_get_binding(DMA_DEVICE_NAME);

	if (!dma_device) {
		printk("Cannot get dma controller\n");
		return -1;
	}

	/* prepare dma cfg params */
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.source_burst_length = blen;
	dma_cfg.dest_burst_length = blen;
	dma_cfg.dma_callback = test_done;
	dma_cfg.complete_callback_en = 1;
	dma_cfg.error_callback_en = 1;
	/*
	 * in this case, set block_count to total, and head_block as head of
	 * LinkedList header pointer, dma will return when all are done
	 */
	dma_cfg.block_count = block_count;
	dma_cfg.head_block = &(dma_block_cfg[0]);

	printk("\nPreparing DMA Controller: Chan_ID=%u, BURST_LEN=%u\n",
	       chan_id, blen);

	/* clean rx buffer */
	memset(rx_data, 0, sizeof(rx_data));
	memset(rx_data2, 0, sizeof(rx_data2));
	memset(rx_data3, 0, sizeof(rx_data3));
	memset(rx_data4, 0, sizeof(rx_data4));
	SCB_CleanDCache_by_Addr((uint32_t *)rx_data, RX_BUFF_SIZE * 4 + 32);

	if (dma_config(dma_device, chan_id, &dma_cfg)) {
		printk("ERROR: configuring\n");
		return -1;
	}

	printk("Starting the transfer\n");

	current_block_count = block_count;
	total_block_count = block_count;

	/* start dma transaction */
	if (dma_start(dma_device, chan_id)) {
		printk("ERROR: transfer\n");
		return -1;
	}

	/* Wait a while for the dma to complete */
	if (k_sem_take(&dma_sem, K_MSEC(WAITTIME))) {
		printk("*** timed out waiting for dma to complete ***\n");
		return -1;
	}

	/* Invalidate cache of rx buffer */
	SCB_InvalidateDCache_by_Addr((uint32_t *)rx_data,
				     RX_BUFF_SIZE * 4 + 32);
	dma_stop(dma_device, chan_id);
	printk("block count = %d\n", block_count);

	/*
	 * compare the rx and tx buffer string, to check whether the dma copy is
	 * done successfully or not.
	 * if compared as the same, it means the dma is working properly, return
	 * 0, else, return -1, indicating there is something wrong
	 */
	switch (block_count) {
	case 4:
		if (strcmp(tx_data4, rx_data4) != 0) {
			return -1;
		}
		printk("%s\n", rx_data4);
	case 3:
		if (strcmp(tx_data3, rx_data3) != 0) {
			return -1;
		}
		printk("%s\n", rx_data3);

	case 2:
		if (strcmp(tx_data2, rx_data2) != 0) {
			return -1;
		}
		printk("%s\n", rx_data2);
	case 1:
		if (strcmp(tx_data, rx_data) != 0) {
			return -1;
		}
		printk("%s\n", rx_data);
		break;

	default:
		printk("Invalid block count %d\n", block_count);
		return -1;
	}

	return 0;
}

/* main entry that runs the samples */
void main(void)
{
	/* wait fw totally boot */
	k_msleep(1000);

	printk("testing DMA normal reload mode\n");
	for (int block_c = 1; block_c <= MAX_TRANSFERS; block_c++) {
		if (test_task(0, BURST_LEN, block_c) == 0) {
			printk("DMA Passed\n");
		} else {
			printk("DMA Failed\n");
		}
		k_msleep(SLEEPTIME);
	}

	printk("\ntesting DMA linked list mode\n");
	for (int block_c = 1; block_c <= MAX_TRANSFERS; block_c++) {
		if (test_ll_task(0, BURST_LEN, block_c) == 0) {
			printk("DMA Passed\n");
		} else {
			printk("DMA Failed\n");
		}
		k_msleep(SLEEPTIME);
	}
}

/**
 * @}
 */


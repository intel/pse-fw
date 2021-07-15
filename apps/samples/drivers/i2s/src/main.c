/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file
 * @brief Sample Application for I2S (Inter-IC Sound) bus driver.
 * This example demonstrate how to use I2S bus driver to transmit stereo
 * data over I2S bus. Here I2S driver is configured to 48Khz sampling
 * frequency with 16bit stereo (2 channel). The sample application will
 * transmit the data for 10 second and then stop transmission.
 *
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more
 * details how to build
 * the sample codes.
 *
 * @brief Hardware setup.
 * Please refer section 3.1.1 in “IntelPSE_SDK_User_Guide”
 * for more details for I2S
 * hardware setup.
 * @{
 */

/* Local Includes */
#include <drivers/i2s.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <sedi.h>
#include <string.h>
#include <drivers/uart.h>
#include <zephyr.h>

/* Number of receive Data block */
#define NUM_RX_BLOCKS 16
/* Number of transmit Data block */
#define NUM_TX_BLOCKS 16
/* Sample No */
#define SAMPLE_NO 32
/* Timeout for I2S transmit request in millisecond */
#define TIMEOUT 2000
/* Audio Sampling frequency */
#define FRAME_CLK_FREQ (48000)
/* Audio sample bit widths */
#define BITS_PER_SAMPLE_16 (16)
/* Number of channels */
#define NUM_CHANNELS_STEREO (2)
/* Block size of each block, measured in bytes */
#define BLOCK_SIZE (4 * 1024)
/* Left channel data */
#define PATTRN_CHNL_0 (0xAAAA)
/* Right channel data */
#define PATTRN_CHNL_1 (0xBBBB)

#define CONFIG_I2S_FORMAT(i2s_cfg, bits, asf, chn, frmt, mod) \
	{						      \
		i2s_cfg.word_size = bits;		      \
		i2s_cfg.channels = chn;			      \
		i2s_cfg.format = frmt;			      \
		i2s_cfg.options = mod;			      \
		i2s_cfg.frame_clk_freq = asf;		      \
	}
#define CONFIG_I2S_BUFF(i2s_cfg, time_out, mem_slb, size) \
	{						  \
		i2s_cfg.block_size = size;		  \
		i2s_cfg.mem_slab = &mem_slb;		  \
		i2s_cfg.timeout = time_out;		  \
	}

/* I2S device name. PSE support two instance, namely I2S_0 and I2S_1 */
char *I2S_DEV_NAME = "I2S_0";

/* Pointers to I2S device handler */
static const struct device *dev_tx;

/* Data buffer containing Left and right channel data,
 * transmit memory slab and task_id
 */
K_MEM_SLAB_DEFINE(tx_1_mem_slab, BLOCK_SIZE, NUM_TX_BLOCKS, 2);
uint16_t aud_buff[BLOCK_SIZE];

/* I2S device configuration parameters for
 * data transfer
 */
struct i2s_app_cfg {
	char *dev_name;
	uint32_t mode;
	uint32_t asf;
	uint32_t bits;
	uint32_t chnl;
};

/* @brief main function
 * Transfer data using I2S device in Philips I2S mode
 */
int main(int argc, char *argv[])
{
	struct i2s_config i2s_cfg;
	int ret;
	uint32_t time_count, i;

	printk("I2S sample application\n");

	memset(&i2s_cfg, 0, sizeof(i2s_cfg));

	/* Get the device handle for I2S device by 'dev_name' */
	dev_tx = device_get_binding(I2S_DEV_NAME);
	if (!dev_tx) {
		printk("I2S: %s Device driver not found\n", I2S_DEV_NAME);
		ret = -1;
		goto exit;
	}
	printk("I2S: %s binding successful\n", I2S_DEV_NAME);

	/* Fill buffer with left and right channel data to transfer
	 * over I2S device. Left channel data is 0xAAAA and right channel
	 * data is 0xBBBB.
	 */
	for (i = 0; i < BLOCK_SIZE; i++) {
		aud_buff[i] = PATTRN_CHNL_0;
		aud_buff[++i] = PATTRN_CHNL_1;
	}

	/* I2S Module configuration
	 * I2S_MODULE(DEVICE) : I2S_DEV_NAME:I2S-0
	 * I2S DATA FORMAT    : I2S_FMT_DATA_FORMAT_I2S
	 * FRAME CLK FREQ     : 48kHz
	 * BITS PER SAMPLE    : 16
	 * NUM OF CHANNELS    : 2
	 */
	CONFIG_I2S_FORMAT(i2s_cfg, BITS_PER_SAMPLE_16, FRAME_CLK_FREQ,
			  NUM_CHANNELS_STEREO, I2S_FMT_DATA_FORMAT_I2S,
			  (I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER |
			   I2S_OPT_PINGPONG));
	CONFIG_I2S_BUFF(i2s_cfg, TIMEOUT, tx_1_mem_slab, BLOCK_SIZE);

	/* Configure operational parameters for I2S module */
	ret = i2s_configure(dev_tx, I2S_DIR_TX, &i2s_cfg);
	if (ret != 0) {
		printk("error in i2s_tx_configure\n");
		ret = -1;
		goto exit;
	}

	/* Transfer data over I2S device(Current configuration: I2S-0) */
	printk("i2s_buf_write2\n");
	ret = i2s_write(dev_tx, aud_buff, BLOCK_SIZE);
	if (ret != 0) {
		printk("error in i2s_write\n");
		ret = -1;
		goto exit;
	}
	/* Readying I2S device for data transfer */
	ret = i2s_trigger(dev_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret != 0) {
		printk("error in i2s_trigger_start for Tx\n");
		ret = -1;
		goto exit;
	}

	/* Transfer data over certain amount of seconds so that I2S data
	 * signals can be seen on data probing device
	 */
	time_count = 4000;
	while (time_count--) {
		/* Transfer data over I2S device */
		ret = i2s_write(dev_tx, aud_buff, BLOCK_SIZE);
	}

	/* Stop data transfer over I2S device, data transfer
	 * stops after current block is finished
	 */
	printk("I2S_TRIGGER_STOP and reconfigure\n");
	ret = i2s_trigger(dev_tx, I2S_DIR_TX, I2S_TRIGGER_STOP);
	if (ret != 0) {
		printk("error in i2s_trigger_stop for Tx\n");
		ret = -1;
		goto exit;
	}

	/* Stop data transfer immediately over I2S device */
	printk("I2S_TRIGGER_DROP and reconfigure\n");
	ret = i2s_trigger(dev_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
	if (ret != 0) {
		printk("error in i2s_trigger_drop for Tx\n");
		ret = -1;
		goto exit;
	}

exit:
	return ret;
}
/**
 * @}
 */

/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This application tests I2S driver for ehl_pse_crb target.
 *
 * @file
 * @brief Sample Application for codec playback
 * and demonstrate how to use CODEC and I2S interface
 * to transmit audio data which is in hex format
 * in test_audio.h
 */

/* Local Includes */
#include <zephyr.h>
#include <sedi.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <drivers/uart.h>
#include <drivers/i2s.h>
#include <audio/codec.h>
#include <string.h>
#include "test_audio.h"

/** Number of transmit Data block */
#define NUM_TX_BLOCKS 16
/** Timeout for I2S transmit request in millisecond */
#define TIMEOUT 2000
/** Sampling frequency */
#define FRAME_CLK_FREQ (16000)
/** Bits per sample */
#define BITS_PER_SAMPLE_16 (16)
/** Number of channels */
#define NUM_CHANNELS_STEREO (2)
/** block size of each block, measured in bytes */
#define BLOCK_SIZE (4 * 1024)
/** Audio data in hex format */
#define RAW_DATA test_audio

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

/* I2S device name */
#define I2S_0_NAME "I2S_0"
/* Codec device name */
#define ALC_DEV_NAME "DT_ALC5660I_LABEL"

/* Pointers to I2S device handler */
const struct device *dev_tx;
/* Pointer to codec device handler */
const struct device *dev_codec;

/* Data buffer containing Left and right channel data,
 * transmit memory slab and task_id
 */
K_MEM_SLAB_DEFINE(tx_1_mem_slab, BLOCK_SIZE, NUM_TX_BLOCKS, 2);

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

/* @brief i2s device configuration and data transfer
 *
 * I2S device is configured for operation based on input parameters
 * and known set of data is transferred over I2S bus
 */
static int test_i2s_mode_tx(char *dev_name, uint32_t mode, uint32_t asf,
			    uint32_t bits, uint32_t chnl)
{
	uint32_t i, audio_cnt;
	int ret;
	struct i2s_config i2s_cfg;

	memset(&i2s_cfg, 0, sizeof(i2s_cfg));

	printk("%s sample Freq:%u, bits:%u, channel:%u\n", __func__,
	       asf, bits, chnl);

	/* Get the device handle for I2S device by I2S_DEV_NAME */
	dev_tx = device_get_binding(I2S_0_NAME);
	if (!dev_tx) {
		printk("I2S: %s Device driver not found\n", I2S_0_NAME);
		ret = -1;
		goto exit;
	}
	printk("I2S: %s binding successful\n", I2S_0_NAME);

	/* I2S Module configuration
	 * I2S_MODULE(DEVICE) : I2S_DEV_NAME:I2S-0
	 * I2S DATA FORMAT    : I2S_FMT_DATA_FORMAT_I2S
	 * FRAME CLK FREQ     : 16kHz
	 * BITS PER SAMPLE    : 16
	 * NUM OF CHANNELS    : 2
	 */
	CONFIG_I2S_FORMAT(
		i2s_cfg, BITS_PER_SAMPLE_16, FRAME_CLK_FREQ,
		NUM_CHANNELS_STEREO, I2S_FMT_DATA_FORMAT_I2S,
		(I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER |
		 I2S_OPT_PINGPONG));

	CONFIG_I2S_BUFF(i2s_cfg, TIMEOUT, tx_1_mem_slab,
			BLOCK_SIZE);

	/* Configure operational parameters for I2S module */
	ret = i2s_configure(dev_tx, I2S_DIR_TX, &i2s_cfg);
	if (ret != 0) {
		printk("error in i2s_tx_configure\n");
		ret = -1;
		goto exit;
	}

	audio_cnt = sizeof(RAW_DATA) / BLOCK_SIZE;

	/* Transfer data over I2S device */
	printk("i2s_buf_write2\n");
	ret = i2s_write(dev_tx, RAW_DATA, BLOCK_SIZE);
	if (ret != 0) {
		printk("Error in i2s_write\n");
		ret = -1;
		goto exit;
	}
	/* Readying I2S device for data transfer */
	ret = i2s_trigger(dev_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret != 0) {
		printk("Error in i2s_trigger_start for Tx\n");
		ret = -1;
		goto exit;
	}

	while (1) {
		/* Transfer audio data over I2S device block by block */
		for (i = 0; i < audio_cnt; i++) {
			ret = i2s_write(dev_tx, RAW_DATA + i * BLOCK_SIZE,
					BLOCK_SIZE);
			if (ret != 0) {
				break;
			}
		}
		if (sizeof(RAW_DATA) % BLOCK_SIZE) {
			ret = i2s_write(dev_tx, RAW_DATA + i * BLOCK_SIZE,
					(sizeof(RAW_DATA) % BLOCK_SIZE));
		}
		if (ret != 0) {
			break;
		}
	}
	/* Stop data transfer over I2S device, data transfer
	 * stops after current block is finished
	 */
	printk("I2S_TRIGGER_STOP and reconfigure\n");
	ret = i2s_trigger(dev_tx, I2S_DIR_TX, I2S_TRIGGER_STOP);
	if (ret != 0) {
		printk("error in i2s_trigger_stop for Tx\n");
		return -1;
	}
	/* Stop data transfer immediately over I2S device */
	printk("I2S_TRIGGER_DROP and reconfigure\n");
	ret = i2s_trigger(dev_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
	if (ret != 0) {
		printk("error in i2s_trigger_drop for Tx\n");
		return -1;
	}
exit:
	return ret;
}

/* @brief main function
 * Transfer audio data using codec device and I2S interface
 * Here audio signal is transmitted through I2S bus
 * to speaker output
 */
int main(int argc, char *argv[])
{
	int ret;
	struct audio_codec_cfg codec_cfg;

	printk("ALC CODEC sample application\n");
	/* Get the device handle for alc codec device by name */
	dev_codec = device_get_binding(ALC_DEV_NAME);
	if (!dev_codec) {
		printk("ALC CODEC: %s Device driver not found\n",
		       ALC_DEV_NAME);
		ret = -1;
		goto exit;
	}
	memset(&codec_cfg, 0, sizeof(codec_cfg));

	/* Codec Module configuration
	 * Codec dia type     : AUDIO_DAI_TYPE_I2S
	 * I2S WORD SIZE      : AUDIO_PCM_WIDTH_16_BITS
	 * I2S CHANNEL        : 1
	 * I2S FORMAT         : I2S_FMT_DATA_FORMAT_I2S
	 * I2S OPTIONS        : I2S_OPT_FRAME_CLK_SLAVE
	 * I2S FRAME CLK FREQ : AUDIO_PCM_RATE_16K
	 */
	codec_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	codec_cfg.dai_cfg.i2s.word_size = AUDIO_PCM_WIDTH_16_BITS;
	codec_cfg.dai_cfg.i2s.channels = 1;
	codec_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	codec_cfg.dai_cfg.i2s.options =
		I2S_OPT_BIT_CLK_SLAVE | I2S_OPT_FRAME_CLK_SLAVE;
	codec_cfg.dai_cfg.i2s.frame_clk_freq = AUDIO_PCM_RATE_16K;

	ret = audio_codec_configure(dev_codec, &codec_cfg);
	if (ret != 0) {
		printk("ALC CODEC: %s code configuration is unsuccessful\n",
		       ALC_DEV_NAME);
		ret = -1;
		goto exit;
	} else {
		printk("ALC CODEC: %s code configuration is successful\n",
		       ALC_DEV_NAME);
	}

	/* I2S Module configuration for testing
	 * I2S_MODULE(DEVICE) : I2S_DEV_NAME:I2S-0
	 * I2S DATA FORMAT    : I2S_FMT_DATA_FORMAT_I2S
	 * FRAME CLK FREQ     : 16kHz
	 * BITS PER SAMPLE    : 16
	 * NUM OF CHANNELS    : 2
	 */
	ret =
		test_i2s_mode_tx(I2S_0_NAME, I2S_FMT_DATA_FORMAT_I2S,
				 FRAME_CLK_FREQ, BITS_PER_SAMPLE_16,
				 NUM_CHANNELS_STEREO);
exit:
	return ret;
}

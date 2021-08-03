/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file
 * @brief Sample Application for codec capture path
 * and to demonstrate how to use codec and I2S interface
 * to capture and play audio data simultaneously
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

/* Number of transmit Data block */
#define NUM_TX_BLOCKS 16
/* Number of receive Data block */
#define NUM_RX_BLOCKS 16
/* Timeout for I2S transmit request in millisecond */
#define TIMEOUT 2000
#define SAMPLE_NO 32
/* Audio Sampling frequency */
#define FRAME_CLK_FREQ (16000)
/* Audio sample bit widths */
#define BITS_PER_SAMPLE_16 (16)
/* Number of channels */
#define NUM_CHANNELS_STEREO (2)
/* Block size of each block, measured in bytes */
#define BLOCK_SIZE (4 * 1024)

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

/* Stack size */
#define I2S_STACK_SIZE 16000
/* Tx thread priority */
#define I2S_PRIORITY_TX 6
/* RX thread priority */
#define I2S_PRIORITY_RX 5
#define MIC_MSGQ_LEN 10
#define RX_MSGQ_LEN 2
#define MSG_Q_ALIGN 4
#define K_TIMEOUT 5

K_THREAD_STACK_DEFINE(i2s_tx_stack_area, I2S_STACK_SIZE);
K_THREAD_STACK_DEFINE(i2s_rx_stack_area, I2S_STACK_SIZE);
struct k_thread tx_thread, rx_thread;

#define CREATE_I2S_TX_THREAD(test_tx_thread, handle, arg1)	  \
	k_thread_create(&handle, i2s_tx_stack_area,		  \
			K_THREAD_STACK_SIZEOF(i2s_tx_stack_area), \
			test_tx_thread,				  \
			arg1, NULL, NULL,			  \
			I2S_PRIORITY_TX, 0, K_NO_WAIT)

#define CREATE_I2S_RX_THREAD(test_rx_thread, handle, arg1)	  \
	k_thread_create(&handle, i2s_rx_stack_area,		  \
			K_THREAD_STACK_SIZEOF(i2s_rx_stack_area), \
			test_rx_thread,				  \
			arg1, NULL, NULL,			  \
			I2S_PRIORITY_RX, 0, K_NO_WAIT)		  \
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
K_MEM_SLAB_DEFINE(rx_1_mem_slab, BLOCK_SIZE, NUM_RX_BLOCKS, 2);

extern struct k_mem_slab tx_1_mem_slab;
extern struct k_mem_slab rx_1_mem_slab;

uint16_t aud_buff[BLOCK_SIZE];
uint16_t aud_buff_rx[BLOCK_SIZE];
uint16_t aud_buff_pong_rx[BLOCK_SIZE];
static uint16_t *captr_ptr;
size_t read_data_cnt;

struct mic_data_item {
	uint8_t *data_ptr;
	uint32_t data_cnt;
} mic_read_data_item;

K_MSGQ_DEFINE(mic_data_msgq, sizeof(mic_read_data_item), MIC_MSGQ_LEN,
	      MSG_Q_ALIGN);
K_MSGQ_DEFINE(rxt_data_msgq, sizeof(mic_read_data_item), RX_MSGQ_LEN,
	      MSG_Q_ALIGN);

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
static int thread_tx(char *dev_name, uint32_t mode, uint32_t asf,
		     uint32_t bits, uint32_t chnl)
{
	int ret;
	struct i2s_config i2s_cfg;

	static struct mic_data_item playback_mic_data;

	memset(&i2s_cfg, 0, sizeof(i2s_cfg));

	printk("%s sample Freq:%u, bits:%u, channel:%u\n",  __func__,
	       asf, bits, chnl);

	/* Get the device handle for I2S device by 'ALC_DEV_NAME' */
	dev_tx = device_get_binding(I2S_0_NAME);
	if (!dev_tx) {
		printk("I2S: %s Device driver not found\n",
		       I2S_0_NAME);
		ret = -1;
		goto exit;
	}
	printk("I2S: %s binding successful\n", I2S_0_NAME);

	/* Set operational parameters for I2S module:
	 * I2S_MODULE(DEVICE) : I2S_DEV_NAME:I2S-0
	 * I2S DATA FORMAT    : I2S_FMT_DATA_FORMAT_I2S
	 * FRAME CLK FREQ     : 16kHz
	 * BITS PER SAMPLE    : 16
	 * NUM OF CHANNELS    : 2
	 * Options            : PINGPONG, I2S MASTER MODE
	 */
	CONFIG_I2S_FORMAT(i2s_cfg, bits, asf, chnl, mode,
			  (I2S_OPT_FRAME_CLK_MASTER |
			   I2S_OPT_BIT_CLK_MASTER  |
			   I2S_OPT_PINGPONG));
	CONFIG_I2S_BUFF(i2s_cfg, TIMEOUT, tx_1_mem_slab, BLOCK_SIZE);

	/* Configure operational parameters for I2S module */
	ret = i2s_configure(dev_tx, I2S_DIR_TX, &i2s_cfg);
	if (ret != 0) {
		printk("error in i2s_tx_configure\n");
		ret = -1;
		goto exit;
	}

	/* Transfer data over I2S device*/
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
	while (1) {
		/* wait on a queue and transmit the audio data
		 * received from rx memory block
		 */
		k_msgq_get(&mic_data_msgq, &playback_mic_data,
			   K_FOREVER);
		ret = i2s_write(dev_tx, playback_mic_data.data_ptr,
				playback_mic_data.data_cnt);
		if (ret != 0) {
			break;
		}
		if (k_msgq_put(&rxt_data_msgq, &playback_mic_data,
			       K_NO_WAIT) != 0) {
			printk("Adding to rx data mesg failed\n");
		}
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

	/* Stop data transfer immediatey over I2S device */
	printk("I2S_TRIGGER_DROP and reconfigure\n");
	ret = i2s_trigger(dev_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
	if (ret != 0) {
		printk("error in i2s_trigger for Tx\n");
		ret = -1;
		goto exit;
	}
	return 0;
exit:
	return ret;
}

/* @brief i2s device configuration and data transfer
 *
 * I2S device is configured for operation based on input parameters
 * and known set of audio data is transferred over I2S bus
 */
int thread_rx(char *dev_name, uint32_t mode, uint32_t asf,
	      uint32_t bits, uint32_t chnl)
{
	int ret;
	struct i2s_config i2s_cfg;
	static struct mic_data_item mic_read_data;
	static uint32_t toggle;

	memset(&i2s_cfg, 0, sizeof(i2s_cfg));

	printk("%s sample Freq:%u, bits:%u, channel:%u\n",  __func__, asf,
	       bits, chnl);

	/* Get the device handle for I2S device by I2S_0_NAME */
	dev_tx = device_get_binding(I2S_0_NAME);
	if (!dev_tx) {
		printk("I2S: %s Device driver not found\n",
		       I2S_0_NAME);
		ret = -1;
		goto exit;
	}
	printk("I2S: %s binding successful\n", I2S_0_NAME);

	/* Set operational parameters for I2S module:
	 * I2S_MODULE(DEVICE) : I2S_DEV_NAME:I2S-0
	 * I2S DATA FORMAT    : I2S_FMT_DATA_FORMAT_I2S
	 * FRAME CLK FREQ     : 16kHz
	 * BITS PER SAMPLE    : 16
	 * NUM OF CHANNELS    : 2
	 * OPTIONS            : PINGPONG, I2S MASTER MODE
	 */
	CONFIG_I2S_FORMAT(i2s_cfg, bits, asf, chnl, mode,
			  (I2S_OPT_FRAME_CLK_MASTER |
			   I2S_OPT_BIT_CLK_MASTER
			   | I2S_OPT_PINGPONG));
	CONFIG_I2S_BUFF(i2s_cfg, TIMEOUT, rx_1_mem_slab, BLOCK_SIZE);

	/* Configure operational parameters for I2S module */
	ret = i2s_configure(dev_tx, I2S_DIR_RX, &i2s_cfg);
	if (ret != 0) {
		printk("error in i2s_rx_configure\n");
		ret = -1;
		goto exit;
	}

	/* Readying I2S device for data transfer */
	ret = i2s_trigger(dev_tx, I2S_DIR_RX, I2S_TRIGGER_START);
	if (ret != 0) {
		printk("error in i2s_trigger_start for Tx\n");
		ret = -1;
		goto exit;
	}

	printk("Entering infinate loop\n ");
	while (1) {
		/* Read the audio data from memory block */
		ret = i2s_read(dev_tx, (void *)&captr_ptr,
			       &read_data_cnt);
		if (ret != 0) {
			break;
		}
		if (!toggle) {
			memcpy(aud_buff_rx, captr_ptr, read_data_cnt);
			mic_read_data.data_ptr = (void *)&aud_buff_rx;
			toggle = 1;
		} else {
			memcpy(aud_buff_pong_rx, captr_ptr, read_data_cnt);
			mic_read_data.data_ptr = (void *)&aud_buff_pong_rx;
			toggle = 0;
		}
		/* put the audio data to mic data message queue */
		while (k_msgq_put(&mic_data_msgq, &mic_read_data,
				  K_NO_WAIT) != 0) {
			/* message queue is full: purge old data & try again */
			k_msgq_purge(&mic_data_msgq);
		}
		k_msgq_get(&rxt_data_msgq, &mic_read_data,
			   SYS_TIMEOUT_MS(K_TIMEOUT));
		k_mem_slab_free(&rx_1_mem_slab, (void *)&captr_ptr);
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
	return 0;
exit:
	return ret;
}

void test_tx_thread(void *arg1, void *arg2, void *arg3)
{
	struct i2s_app_cfg *tx_cfg = (struct i2s_app_cfg *)arg1;

	thread_tx(tx_cfg->dev_name, tx_cfg->mode,
		  tx_cfg->asf, tx_cfg->bits, tx_cfg->chnl);
}

void test_rx_thread(void *arg1, void *arg2, void *arg3)
{
	struct i2s_app_cfg *rx_cfg = (struct i2s_app_cfg *)arg1;

	thread_rx(rx_cfg->dev_name, rx_cfg->mode,
		  rx_cfg->asf, rx_cfg->bits, rx_cfg->chnl);
}

static void loopback_capture(char *dev_name, uint32_t mode, uint32_t asf,
			     uint32_t bits, uint32_t chnl)
{

	static struct i2s_app_cfg loop_config;

	loop_config.dev_name = dev_name;
	loop_config.mode = mode;
	loop_config.asf = asf;
	loop_config.bits = bits;
	loop_config.chnl = chnl;

	CREATE_I2S_TX_THREAD(test_tx_thread, tx_thread, &loop_config);
	CREATE_I2S_RX_THREAD(test_rx_thread, rx_thread, &loop_config);

}

/* @brief main function
 *
 * Capture and play audio data using codec device
 * and I2S interface.
 * Capture audio by playing music from laptop through audio
 * input and play the same audio through speakers
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
	loopback_capture(I2S_0_NAME, I2S_FMT_DATA_FORMAT_I2S,
			 FRAME_CLK_FREQ, BITS_PER_SAMPLE_16,
			 NUM_CHANNELS_STEREO);
exit:
	return ret;
}
/**
 * @}
 */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

#include <drivers/video.h>
#include <drivers/uart.h>
#include <device.h>
#include <drivers/ipm.h>

#include <string.h>

#define VIDEO_DEV DT_LABEL(DT_INST(0, byd_bf30a2))

#define UART2_DEV DT_LABEL(DT_INST(0, listenai_csk_uart))

void dump(const struct device *dev, char *buffer, uint32_t l)
{
	while(l--) {
		uart_poll_out(dev, *buffer);
		buffer++;
	}
}
typedef void (msg_cb_t)(void);


static void ipm_callback(const struct device *dev, void *context,
			 uint32_t id, volatile void *data)
{
	printk("id=%d.\n", id);
}

#define BF30A2_FRAME_WIDTH (128)
#define BF30A2_FRAME_HEIGHT (180)

#define BF30A2_DEPTH (1)
#define BF30A2_RX_BUF_CNT (2)

#define BF30A2_FRAME_START_PACKET_LEN (9)
#define BF30A2_LINE_START_PACKET_LEN (6)
#define BF30A2_DATA_PACKET_HEADA_LEN (6)

#define BF30A2_DATA_PACKET_LEN                                      	\
	((BF30A2_LINE_START_PACKET_LEN + BF30A2_DATA_PACKET_HEADA_LEN + 	\
			BF30A2_FRAME_WIDTH * BF30A2_DEPTH))

#define BF30A2_FRAME_END_PACKET_LEN (4)

#define BF30A2_FRAME_SIZE                                                            \
	(BF30A2_FRAME_START_PACKET_LEN + BF30A2_DATA_PACKET_LEN * BF30A2_FRAME_HEIGHT +  \
			 BF30A2_LINE_START_PACKET_LEN + BF30A2_FRAME_END_PACKET_LEN)

void main(void)
{
	struct video_buffer *vbuf;
	printk("Hello World! %s\n", CONFIG_BOARD);
	int ret;

	const struct device *video = device_get_binding(VIDEO_DEV);
	const struct device *uart2 = device_get_binding(UART2_DEV);
	const struct device *ipm = DEVICE_DT_GET(DT_NODELABEL(mailbox0));

	if (!device_is_ready(ipm)) {
		printk("ipm device is null.\n");
		return;
	}

	if (video == NULL) {
		printk("video device is null.\n");
		return;
	}

	if (uart2 == NULL) {
		printk("uart2 device is null.\n");
		return;
	}

	ipm_register_callback(ipm, ipm_callback, NULL);
	ipm_set_enabled(ipm, 1);
	int x = 0;
	ipm_send(ipm, 1, 0, &x, sizeof(x));
	/* Start video capture */
	if (video_stream_start(video)) {
		printk("Unable to start video.\n");
		return;
	}
	printk("start video steam.\n");

	uint32_t last_tick = 0;
	while(1) 
	{
		ret = video_dequeue(video, VIDEO_EP_OUT, &vbuf, K_FOREVER);
		if (ret == 0) {
			if (vbuf->timestamp - last_tick > 0) {
				printk("FPS:%d.\n", 1000 / (vbuf->timestamp - last_tick));
			}
			last_tick = vbuf->timestamp;
		}
	}
}

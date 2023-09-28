/*
 * Copyright (c) 2023, LISTENAI
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <zephyr/usb/usb_device.h>
#include <stdint.h>

typedef struct {
	uint8_t magic[4];
	uint16_t action;
	uint32_t length;
	uint16_t checksum;
} bitmap_header_t;

typedef struct {
	int x;
	int y;
	int w;
	int h;
	float score;
	int detect_loss_cnt;
	int id;
	int loss_cnt_thres;
	int update;
	float iou_score;
} shoulder_t;

typedef struct {
	int gesture;
	float score;
	int status;
	int x;
	int y;
	int w;
	int h;
	int id;
} gesture_t;

typedef struct {
	int x;
	int y;
	int w;
	int h;
	int id;
	float detect_score;
	int feature_dim;
} face_t;

typedef enum {
	FORMAT_RGB565 = 0,
	FORMAT_RGB888 = 1,
	FORMAT_BGR888 = 2,
	FORMAT_YUYV_422 = 3,
	FORMAT_YVYU_422 = 4,
	FORMAT_VYUY_422 = 5,
	FORMAT_UYVY_422 = 6
} image_format_e;

typedef struct {
	int width;
	int height;
	int format;
	float scale;
} img_info_t;

typedef struct {
	uint32_t event;
	char *text;
} directive_t;

typedef enum {
	e_action_null = 0,
	e_action_ready = 1,
	e_action_state = 2,
	e_action_bitmap = 30,
	e_action_face = 31,
	e_action_shoulder = 32,
	e_action_position = 33,
	e_action_motor = 34,
	e_action_gesture = 35,
	e_action_image_info = 36,
	e_action_msg = 37,
	e_action_directive = 38
} bitmap_action_e;

#define HEADER_CHECKSUM_BYPASS 0xFFFF
#define BITMAP_HEADER_LEN      16

static inline bitmap_header_t create_header(uint16_t action, uint32_t data_len)
{
	bitmap_header_t hdr = {.magic = {'C', 'U', 'P', '2'}, // CSK USB PROTOCOL V2
			       .action = action,
			       .length = data_len,
			       .checksum = HEADER_CHECKSUM_BYPASS};
	return hdr;
}

int webusb_gcl_send(uint8_t *data, uint32_t data_len, bitmap_action_e action);

int webusb_gcl_send_bitmap(uint8_t *data, uint32_t data_len);

int webusb_gcl_send_shoulder(uint8_t *data, uint32_t data_len);

int webusb_gcl_send_gesture(uint8_t *data, uint32_t data_len);

int webusb_gcl_send_face(face_t face, char *text);

int webusb_gcl_send_image_info(img_info_t img_info);

int webusb_gcl_send_message(char *text);

int webusb_gcl_register_directive(directive_t directive);

#endif
/*
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/video-controls.h>

#define VIDEO_PIX_FMT_BGR24 video_fourcc('B', 'G', 'R', '3') /* 24  BGR-8-8-8 */
#define VIDEO_PIX_FMT_RGB24 video_fourcc('R', 'G', 'B', '3') /* 24  RGB-8-8-8 */

/* JPEG formats */
#define VIDEO_PIX_FMT_JPEG video_fourcc('J', 'P', 'E', 'G') /*  8  JPEG */

/* YUV formats */
#define VIDEO_PIX_FMT_YUYV video_fourcc('Y', 'U', 'Y', 'V') /* 16  YUYV */
#define VIDEO_PIX_FMT_YVYU video_fourcc('Y', 'V', 'Y', 'U') /* 16  YVYU */
#define VIDEO_PIX_FMT_VYUY video_fourcc('V', 'Y', 'U', 'Y') /* 16  VYUY */
#define VIDEO_PIX_FMT_UYVY video_fourcc('U', 'Y', 'V', 'Y') /* 16  UYVY */

#define VIDEO_CID_INIT (VIDEO_CTRL_CLASS_GENERIC + 2) /* Initialize the camera */

#define VIDEO_CTRL_CLASS_PARAMETERS	 0x00040000 /* Parameters controls */
#define VIDEO_CID_CAMERA_EXPOSURE_TIME	 (VIDEO_CTRL_CLASS_PARAMETERS | VIDEO_CID_CAMERA_EXPOSURE)
#define VIDEO_CID_CAMERA_GAIN_PARAMETERS (VIDEO_CTRL_CLASS_PARAMETERS | VIDEO_CID_CAMERA_GAIN)

#ifdef __cplusplus
}
#endif
/*
 * Copyright (c) 2023, LISTENAI
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __GCL_SERVICE_WEBUSB_H__
#define __GCL_SERVICE_WEBUSB_H__

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <usb_descriptor.h>
#include <zephyr/usb/bos.h>

typedef void (*webusb_gcl_status_cb_t)(struct usb_cfg_data *cfg,
	enum usb_dc_status_code cb_status, const uint8_t *param);

typedef void (*webusb_gcl_handler_data_cb_t)(uint8_t *data, uint32_t len);

/**
 * @brief 初始化 WebUSB gcl 模块
 * 
 * @return 
 * 			GCL_OK : 初始化成功
 * 			-GCL_ERR_INVALID_STATE: 模块已经初始化
 */
int webusb_gcl_init(void);

/**
 * @brief webusb是否枚举成功，需要枚举后才能发送数据
 * 
 * @return
 * 			true : 枚举成功
 * 			false: 没有枚举成功
 */
bool webusb_has_configured(void);

/**
 * @brief 注册 WebUSB gcl 模块 USB 数据处理回调函数
 * 
 * @param cb[in] 数据处理回调函数
 * @return
 * 			GCL_OK : 注册成功
 * 			-GCL_ERR_INVALID_ARG: 无效参数，回调函数为空
 */
int webusb_gcl_register_transfer_recv_handler_callback(webusb_gcl_handler_data_cb_t cb);

struct usb_cfg_data get_usb_cfg();
/**
 * @brief WebUSB gcl 发送 USB 数据包
 * 
 * @param data[in] 需要发送的数据指针
 * @param len[in] 需要发送的数据长度
 * @return
 * 			GCL_OK : 发送成功
 * 			-GCL_ERR_INVALID_ARG: 无效参数，数据指针为空
 */
int webusb_gcl_transfer_sync_send(uint8_t *data, uint32_t len);

/**
 * @brief 注册 WebUSB gcl 逆初始化模块
 * 
 * @return
 * 			GCL_OK : 逆初始化成功
 * 			-GCL_ERR_INVALID_STATE: 模块未初始化
 */
int webusb_gcl_deinit(void);

#endif /* __GCL_SERVICE_WEBUSB_H__ */
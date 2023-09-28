/*
 * Copyright (c) 2023, LISTENAI
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "gcl_webusb.h"
#include "errno_gcp.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gcl_webusb, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_device.h>
#include <usb_descriptor.h>
#include <zephyr/usb/bos.h>

struct usb_cfg_data get_usb_cfg(void);

typedef struct _webusb_gcl_request_handler_t {
	/* Handler for WebUSB Vendor specific commands */
	usb_request_handler vendor_handler;
	/**
	 * The custom request handler gets a first chance at handling
	 * the request before it is handed over to the 'chapter 9' request
	 * handler
	 */
	usb_request_handler custom_handler;
} webusb_gcl_request_handler_t;

/* Max packet size for Bulk endpoints */
#define WEBUSB_BULK_EP_MPS 64
/* Number of interfaces */
#define WEBUSB_NUM_ITF     0x01
/* Number of Endpoints in the custom interface */
#define WEBUSB_NUM_EP      0x02

uint8_t rx_buf[WEBUSB_BULK_EP_MPS];

static webusb_gcl_request_handler_t *req_handlers = NULL;
static bool has_usb_cfg = false;

#define INITIALIZER_IF(num_ep, iface_class)						\
	{															\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,					\
		.bInterfaceNumber = 0,									\
		.bAlternateSetting = 0,									\
		.bNumEndpoints = num_ep,								\
		.bInterfaceClass = iface_class,							\
		.bInterfaceSubClass = 0,								\
		.bInterfaceProtocol = 0,								\
		.iInterface = 0,										\
	}

#define INITIALIZER_IF_EP(addr, attr, mps, interval)			\
	{															\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,					\
		.bEndpointAddress = addr,								\
		.bmAttributes = attr,									\
		.wMaxPacketSize = sys_cpu_to_le16(mps), 				\
		.bInterval = interval,									\
	}

USBD_CLASS_DESCR_DEFINE(primary, 0)

struct {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_out_ep;
} __attribute__((packed)) webusb_desc = {
	.if0 = INITIALIZER_IF(WEBUSB_NUM_EP, USB_BCC_VENDOR),
	.if0_in_ep = INITIALIZER_IF_EP(AUTO_EP_IN, USB_DC_EP_BULK, WEBUSB_BULK_EP_MPS, 0),
	.if0_out_ep = INITIALIZER_IF_EP(AUTO_EP_OUT, USB_DC_EP_BULK, WEBUSB_BULK_EP_MPS, 0),
};

USB_DEVICE_BOS_DESC_DEFINE_CAP struct usb_bos_webusb_desc {
	struct usb_bos_platform_descriptor platform;
	struct usb_bos_capability_webusb cap;
} __attribute__((packed)) bos_cap_webusb = {
	/* WebUSB Platform Capability Descriptor:
	 * https://wicg.github.io/webusb/#webusb-platform-capability-descriptor
	 */
	.platform =
		{
			.bLength = sizeof(struct usb_bos_platform_descriptor) +
				   sizeof(struct usb_bos_capability_webusb),
			.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
			.bDevCapabilityType = USB_BOS_CAPABILITY_PLATFORM,
			.bReserved = 0,
			/* WebUSB Platform Capability UUID
			 * 3408b638-09a9-47a0-8bfd-a0768815b665
			 */
			.PlatformCapabilityUUID =
				{
					0x38, 0xB6, 0x08, 0x34, 0xA9, 0x09, 0xA0, 0x47, \
					0x8B, 0xFD, 0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65,
				},
		},
	.cap = {
		.bcdVersion = sys_cpu_to_le16(0x0100),
		.bVendorCode = 0x01,
		.iLandingPage = 0x01
	}
};

/* Predefined response to control commands related to MS OS 2.0 descriptors */
static const uint8_t msos2_descriptor[] = {
	/* MS OS 2.0 set header descriptor   */
	0x0A, 0x00,                 /* Descriptor size (10 bytes)                 */
	0x00, 0x00,                 /* MS_OS_20_SET_HEADER_DESCRIPTOR             */
	0x00, 0x00, 0x03, 0x06,     /* Windows version (8.1) (0x06030000)         */
	(0x0A + 0x14), 0x00,        /* Length of the MS OS 2.0 descriptor set     */

	/* MS OS 2.0 compatible ID descriptor */
	0x14, 0x00, /* Descriptor size                */
	0x03, 0x00, /* MS_OS_20_FEATURE_COMPATIBLE_ID */
	/* 8-byte compatible ID string, then 8-byte sub-compatible ID string */
	'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

USB_DEVICE_BOS_DESC_DEFINE_CAP struct usb_bos_msosv2_desc {
	struct usb_bos_platform_descriptor platform;
	struct usb_bos_capability_msos cap;
} __attribute__((packed)) bos_cap_msosv2 = {
	/* Microsoft OS 2.0 Platform Capability Descriptor
	 * See https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/
	 * microsoft-defined-usb-descriptors
	 * Adapted from the source:
	 * https://github.com/sowbug/weblight/blob/master/firmware/webusb.c
	 * (BSD-2) Thanks http://janaxelson.com/files/ms_os_20_descriptors.c
	 */
	.platform =
		{
			.bLength = sizeof(struct usb_bos_platform_descriptor) +
				   sizeof(struct usb_bos_capability_msos),
			.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
			.bDevCapabilityType = USB_BOS_CAPABILITY_PLATFORM,
			.bReserved = 0,
			.PlatformCapabilityUUID =
				{
					/**
					 * MS OS 2.0 Platform Capability ID
					 * D8DD60DF-4589-4CC7-9CD2-659D9E648A9F
					 */
					0xDF, 0x60, 0xDD, 0xD8, 0x89, 0x45, 0xC7, 0x4C, \
					0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
				},
		},
	.cap =
		{/* Windows version (8.1) (0x06030000) */
		 .dwWindowsVersion = sys_cpu_to_le32(0x06030000),
		 .wMSOSDescriptorSetTotalLength = sys_cpu_to_le16(sizeof(msos2_descriptor)),
		 .bMS_VendorCode = 0x02,
		 .bAltEnumCode = 0x00},
};

USB_DEVICE_BOS_DESC_DEFINE_CAP struct usb_bos_capability_lpm bos_cap_lpm = {
	.bLength = sizeof(struct usb_bos_capability_lpm),
	.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
	.bDevCapabilityType = USB_BOS_CAPABILITY_EXTENSION,
	/**
	 * BIT(1) - LPM support
	 * BIT(2) - BESL support
	 */
	.bmAttributes = BIT(1) | BIT(2),
};


/* WebUSB Device Requests */
static const uint8_t webusb_allowed_origins[] = {
	/* Allowed Origins Header:
	 * https://wicg.github.io/webusb/#get-allowed-origins
	 */
	0x05, 0x00, 0x0D, 0x00, 0x01,

	/* Configuration Subset Header:
	 * https://wicg.github.io/webusb/#configuration-subset-header
	 */
	0x04, 0x01, 0x01, 0x01,

	/* Function Subset Header:
	 * https://wicg.github.io/webusb/#function-subset-header
	 */
	0x04, 0x02, 0x02, 0x01};

/* Number of allowed origins */
#define NUMBER_OF_ALLOWED_ORIGINS 1

/* Predefined response to control commands related to MS OS 1.0 descriptors
 * Please note that this code only defines "extended compat ID OS feature
 * descriptors" and not "extended properties OS features descriptors"
 */
#define MSOS_STRING_LENGTH 18
static struct string_desc {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bString[MSOS_STRING_LENGTH];

} __packed msos1_string_descriptor = {
	.bLength = MSOS_STRING_LENGTH,
	.bDescriptorType = USB_DESC_STRING,
	/* Signature MSFT100 */
	.bString =
		{
			'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00, '1', 0x00, '0', 0x00, '0', 0x00,
			0x03, /* Vendor Code, used for a control request */
			0x00, /* Padding byte for VendorCode looks like UTF16 */
		},
};

static const uint8_t msos1_compatid_descriptor[] = {
	/* See https://github.com/pbatard/libwdi/wiki/WCID-Devices */
	/* MS OS 1.0 header section */
	0x28, 0x00, 0x00, 0x00,                   /* Descriptor size (40 bytes)          */
	0x00, 0x01,                               /* Version 1.00                        */
	0x04, 0x00,                               /* Type: Extended compat ID descriptor */
	0x01,                                     /* Number of function sections         */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* reserved    */

	/* MS OS 1.0 function section */
	0x02, /* Index of interface this section applies to. */
	0x01, /* reserved */
	/* 8-byte compatible ID string, then 8-byte sub-compatible ID string */
	'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00 /* reserved */
};

/**
 * @brief Custom handler for standard requests in
 *        order to catch the request and return the
 *        WebUSB Platform Capability Descriptor.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail
 */
static int custom_handle_req(struct usb_setup_packet *pSetup, int32_t *len, uint8_t **data)
{
	if (usb_reqtype_is_to_device(pSetup)) {
		return -ENOTSUP;
	}

	if (USB_GET_DESCRIPTOR_TYPE(pSetup->wValue) == USB_DESC_STRING &&
	    USB_GET_DESCRIPTOR_INDEX(pSetup->wValue) == 0xEE) {
		*data = (uint8_t *)(&msos1_string_descriptor);
		*len = sizeof(msos1_string_descriptor);

		LOG_DBG("Get MS OS Descriptor v1 string");

		return 0;
	}

	return -EINVAL;
}

/* URL Descriptor: https://wicg.github.io/webusb/#url-descriptor */
#define WEBUSB_DESCRIPTOR_TYPE 0x03

// #define WEBUSB_URL_SCHEME_HTTP   0x00 //"http://"
// #define WEBUSB_URL_SCHEME_HTTPS  0x01 //"https://"
#define WEBUSB_URL_SCHEME        CONFIG_WEBUSB_URL_SCHEME_TYPE
#define WEBUSB_ORIGIN_URL_STRING CONFIG_WEBUSB_URL

static const char* webusb_url_string = WEBUSB_ORIGIN_URL_STRING;

struct webusb_url_descriptor_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bScheme;
	char URL[sizeof(WEBUSB_ORIGIN_URL_STRING)];
}  __attribute__((packed));
#define WEBUSB_DT_URL_DESCRIPTOR_SIZE 3

static struct webusb_url_descriptor_t webusb_url_descriptor;

/**
 * @brief Handler called for vendor specific commands. This includes
 *        WebUSB allowed origins and MS OS 1.0 and 2.0 descriptors.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
static int vendor_handle_req(struct usb_setup_packet *pSetup, int32_t *len, uint8_t **data)
{
	if (usb_reqtype_is_to_device(pSetup)) {
		return -ENOTSUP;
	}

	/* Get Allowed origins request */
	if (pSetup->bRequest == 0x01 && pSetup->wIndex == 0x01) {
		*data = (uint8_t *)(&webusb_allowed_origins);
		*len = sizeof(webusb_allowed_origins);

		LOG_DBG("Get webusb_allowed_origins");

		return 0;
	} else if (pSetup->bRequest == 0x01 && pSetup->wIndex == 0x02) {
		/* Get URL request */
		uint8_t index = USB_GET_DESCRIPTOR_INDEX(pSetup->wValue);

		if (index == 0U || index > NUMBER_OF_ALLOWED_ORIGINS) {
			return -ENOTSUP;
		}
		webusb_url_descriptor.bLength =
			WEBUSB_DT_URL_DESCRIPTOR_SIZE + strlen(webusb_url_string);
		webusb_url_descriptor.bDescriptorType = WEBUSB_DESCRIPTOR_TYPE;
		webusb_url_descriptor.bScheme = WEBUSB_URL_SCHEME;
		memcpy(&webusb_url_descriptor.URL, webusb_url_string, strlen(webusb_url_string));

		*data = (uint8_t *)(&webusb_url_descriptor);
		*len = webusb_url_descriptor.bLength;
		LOG_HEXDUMP_DBG((uint8_t *)&webusb_url_descriptor, sizeof(webusb_url_descriptor),
				"URL Descriptor");

		return 0;
	} else if (pSetup->bRequest == 0x02 && pSetup->wIndex == 0x07) {
		/* Get MS OS 2.0 Descriptors request */
		/* 0x07 means "MS_OS_20_DESCRIPTOR_INDEX" */
		*data = (uint8_t *)(&msos2_descriptor);
		*len = sizeof(msos2_descriptor);

		LOG_DBG("Get MS OS Descriptors v2");

		return 0;
	} else if (pSetup->bRequest == 0x03 && pSetup->wIndex == 0x04) {
		/* Get MS OS 1.0 Descriptors request */
		/* 0x04 means "Extended compat ID".
		 * Use 0x05 instead for "Extended properties".
		 */
		*data = (uint8_t *)(&msos1_compatid_descriptor);
		*len = sizeof(msos1_compatid_descriptor);

		LOG_DBG("Get MS OS Descriptors CompatibeID");

		return 0;
	}

	return -ENOTSUP;
}

void (*webusb_gcl_status_handler)(struct usb_cfg_data *cfg, enum usb_dc_status_code cb_status,
				  const uint8_t *param) = NULL;

void (*webusb_gcl_transfer_recv_handler)(uint8_t *data, uint32_t len) = NULL;

void _webusb_status_callback(struct usb_cfg_data *cfg, enum usb_dc_status_code status,
			     const uint8_t *param)
{
	/* Check the USB status and do needed action if required */
	LOG_INF("_webusb_status_callback status:%d", status);
	switch (status) {
	case USB_DC_ERROR:
		LOG_DBG("USB device error");
		break;
	case USB_DC_RESET:
		LOG_DBG("USB device reset detected");
		break;
	case USB_DC_CONNECTED:
		// has_usb_cfg = true;
		LOG_DBG("USB device connected");
		break;
	case USB_DC_CONFIGURED:
		LOG_DBG("USB device configured");
		has_usb_cfg = true;
		break;
	case USB_DC_DISCONNECTED:
		LOG_DBG("USB device disconnected");
		has_usb_cfg = false;
		break;
	default:
		LOG_DBG("USB unknown state");
		break;
	}
}

int webusb_gcl_register_status_callback(webusb_gcl_status_cb_t cb)
{
	if (cb == NULL) {
		return -GCL_ERR_INVALID_ARG;
	}

	webusb_gcl_status_handler = cb;
	return GCL_OK;
}

int webusb_gcl_register_transfer_recv_handler_callback(webusb_gcl_handler_data_cb_t cb)
{
	if (cb == NULL) {
		return -GCL_ERR_INVALID_ARG;
	}

	webusb_gcl_transfer_recv_handler = cb;
	return GCL_OK;
}

/**
 * @brief Custom handler for standard requests in order to
 *        catch the request and return the WebUSB Platform
 *        Capability Descriptor.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
int webusb_custom_handle_req(struct usb_setup_packet *pSetup, int32_t *len, uint8_t **data)
{
	/* Call the callback */
	if (req_handlers && req_handlers->custom_handler) {
		return req_handlers->custom_handler(pSetup, len, data);
	}

	return -EINVAL;
}

/**
 * @brief Handler called for WebUSB vendor specific commands.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
static int webusb_vendor_handle_req(struct usb_setup_packet *pSetup, int32_t *len, uint8_t **data)
{
	/* Call the callback */
	if (req_handlers && req_handlers->vendor_handler) {
		return req_handlers->vendor_handler(pSetup, len, data);
	}

	return -EINVAL;
}

static void webusb_write_cb(uint8_t ep, int size, void *priv)
{
	LOG_DBG("ep %x size %u", ep, size);
}

static void webusb_read_cb(uint8_t ep, int size, void *priv)
{
	struct usb_cfg_data *cfg = priv;

	LOG_DBG("cfg %p ep %x size %u", cfg, ep, size);

	if (size <= 0) {
		goto done;
	}

	if (webusb_gcl_transfer_recv_handler && size > 0) {
		webusb_gcl_transfer_recv_handler(rx_buf, size);
	}
#if CONFIG_WEBUSB_URL_LOCALHOST_LOOPBACK_TEST
	usb_transfer(cfg->endpoint[CONFIG_WEBUSB_IN_EP_IDX].ep_addr, rx_buf, size,
		     USB_TRANS_WRITE, webusb_write_cb, cfg);
#endif

done:
	usb_transfer(ep, rx_buf, sizeof(rx_buf), USB_TRANS_READ, webusb_read_cb, cfg);
}

/**
 * @brief Callback used to know the USB connection status
 *
 * @param status USB device status code.
 *
 * @return  N/A.
 */
static void webusb_dev_status_cb(struct usb_cfg_data *cfg, enum usb_dc_status_code status,
				 const uint8_t *param)
{

	if (status == USB_DC_CONFIGURED) {
		// struct usb_cfg_data cfg = get_usb_cfg();
		// uint8_t ep = cfg.endpoint[CONFIG_WEBUSB_OUT_EP_IDX].ep_addr;
		// LOG_DBG("USB configured, start read on EP 0x%x", ep);
		// usb_transfer(ep, rx_buf, sizeof(rx_buf), USB_TRANS_READ, webusb_read_cb, NULL);
		
		LOG_DBG("USB device configured");
		webusb_read_cb(cfg->endpoint[CONFIG_WEBUSB_OUT_EP_IDX].ep_addr,
			       0, cfg);
	}

	if (webusb_gcl_status_handler) {
		webusb_gcl_status_handler(cfg, status, param);
	}
}

/* Describe EndPoints configuration */
static struct usb_ep_cfg_data webusb_ep_data[] = {
	{.ep_cb = usb_transfer_ep_callback, .ep_addr = AUTO_EP_IN},
	{.ep_cb = usb_transfer_ep_callback, .ep_addr = AUTO_EP_OUT}};

USBD_DEFINE_CFG_DATA(webusb_config) = {
	.usb_device_description = NULL,
	.interface_descriptor = &webusb_desc.if0,
	.cb_usb_status = webusb_dev_status_cb,
	.interface =
		{
			.class_handler = NULL,
			.custom_handler = webusb_custom_handle_req,
			.vendor_handler = webusb_vendor_handle_req,
		},
	.num_endpoints = ARRAY_SIZE(webusb_ep_data),
	.endpoint = webusb_ep_data
};

struct usb_cfg_data get_usb_cfg(void)
{
	return webusb_config;
}

int webusb_gcl_init(void)
{
	webusb_gcl_register_status_callback(_webusb_status_callback);
	usb_bos_register_cap((void *)&bos_cap_webusb);
	usb_bos_register_cap((void *)&bos_cap_msosv2);
	usb_bos_register_cap((void *)&bos_cap_lpm);

	/* Custom and Vendor request handlers */
	static webusb_gcl_request_handler_t req = {
		.vendor_handler = vendor_handle_req,
		.custom_handler = custom_handle_req,
	};

	/* Set the custom and vendor request handlers */
	req_handlers = &req;

	// ret = usb_enable(NULL);

	// if (ret < 0) {
	// 	LOG_ERR("Failed to config USB");
	// 	return ret;
	// }

	return GCL_OK;
}

bool webusb_has_configured(void)
{
	return has_usb_cfg;
}

int webusb_gcl_deinit(void)
{
	int ret;

	ret = usb_disable();
	if (ret < 0) {
		LOG_ERR("Failed to config USB, ret: %d", ret);
		return -GCL_ERR_INVALID_STATE;
	}

	webusb_gcl_status_handler = NULL;
	webusb_gcl_transfer_recv_handler = NULL;
	req_handlers = NULL;

	return GCL_OK;
}

int webusb_gcl_transfer_sync_send(uint8_t *data, uint32_t len)
{
	int ret;

	/* Set the custom and vendor request handlers */
	if (req_handlers == NULL) {
		return -GCL_ERR_INVALID_STATE;
	}

	struct usb_cfg_data cfg = get_usb_cfg();
	uint8_t ep_in = cfg.endpoint[CONFIG_WEBUSB_IN_EP_IDX].ep_addr;
	ret = usb_transfer_sync(ep_in, data, len, USB_TRANS_WRITE);

	if (ret != 0) {
		LOG_ERR("Failed to transfer data, ret: %d", ret);
		return -GCL_ERR_COMM_FAIL;
	}

	return -GCL_OK;
}

// #define WEBUSB_CFG_DATA_DEFINE(x)
// 	USBD_CLASS_DESCR_DEFINE(primary, x)
// 	struct usb_webusb_config#x

// #define DT_DRV_COMPAT zephyr_webusb

// #define WEBUSB_DT_DEVICE_DEFINE(idx)						\
// 	BUILD_ASSERT(DT_INST_ON_BUS(idx, usb), 					\
// 		"node " DT_NODE_PATH(DT_DRV_INST(idx))				\
// 		" is not on the usb bus")					\
// 	WEBUSB_CFG_DATA_DEFINE(idx);						\
// 	DEVICE_DT_INST_DEFINE(idx, webusb_init, NULL, 				\
// 			      &webusb_data_##idx, &webusb_config_##idx,		\
// 			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
// 			      &webusb_driver_api);

// DT_INST_FOREACH_STATUS_OKAY(WEBUSB_DT_DEVICE_DEFINE);
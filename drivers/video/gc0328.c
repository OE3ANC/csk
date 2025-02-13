/*
 * Copyright (c) 2022, listenai
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT galaxyc_gc0328

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>

#include <drivers/csk6_video_extend.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(gc0328);

/**
 * @brief  GC0328 ID
 */
#define GC0328_ID 0x9D

struct gc0328_data {
	const struct i2c_dt_spec i2c_bus;
	const struct gpio_dt_spec reset_gpios;
	const struct gpio_dt_spec pwdn_gpios;
	struct video_format fmt;
	uint8_t flip;
	uint8_t mirror;
};

struct gc0328_reg {
	uint8_t addr;
	uint8_t value;
};

// 画面镜像和翻转
#define PAGE_BANK_REG_CID_MIRROR		0x17	// default mirror off
#define PAGE_BANK_REG_CID_FLIP			0x17	// default flip off

#define REG_CID_MIRROR_OFF_VALUE		0x00
#define REG_CID_MIRROR_ON_VALUE			0x01	// 0x17, bit[0],2'b00 mirror off,2'b01 mirror on
#define REG_CID_FLIP_OFF_VALUE			0x00
#define REG_CID_FLIP_ON_VALUE			0x02	// 0x17, bit[1],2'b00 flip off,2'b02 flip on

/* 25M CLK & VGA & YUV(UYVY) & 15fps */
static const struct gc0328_reg gc0328_init_reg_tb[] = {
	{0xfe,0x80},
	{0xfe,0x80},
	{0xfc,0x16},
	{0xfc,0x16},
	{0xfc,0x16},
	{0xfc,0x16},
	{0xfe,0x00},
	{0x4f,0x00},
	{0x42,0x00},
	{0x03,0x00},
	{0x04,0xc0},
	{0x77,0x62},
	{0x78,0x40},
	{0x79,0x4d},
	{0x05,0x01},
	{0x06,0x32},
	{0x07,0x00},
	{0x08,0x0c},
	{0xfe,0x01},
	{0x29,0x00},
	{0x2a,0x78},
	{0x2b,0x01},
	{0x2c,0xe0},
	{0x2d,0x01},
	{0x2e,0xe0},
	{0x2f,0x01},
	{0x30,0xe0},
	{0x31,0x01},
	{0x32,0xe0},
	{0xfe,0x00},
	{0xfe,0x01},
	{0x4f,0x00},
	{0x4c,0x01},
	{0xfe,0x00},
	//////////////////////////////
	///////////AWB///////////
	////////////////////////////////
	{0xfe,0x01,},
	{0x51,0x80,},
	{0x52,0x12,},
	{0x53,0x80,},
	{0x54,0x60,},
	{0x55,0x01,},
	{0x56,0x06,},
	{0x5b,0x02,},
	{0x61,0xdc,},
	{0x62,0xdc,},
	{0x7c,0x71,},
	{0x7d,0x00,},
	{0x76,0x00,},
	{0x79,0x20,},
	{0x7b,0x00,},
	{0x70,0xFF,},
	{0x71,0x00,},
	{0x72,0x10,},
	{0x73,0x40,},
	{0x74,0x40,},
	////AWB//
	{0x50,0x00,},
	{0xfe,0x01,},
	{0x4f,0x00,},
	{0x4c,0x01,},
	{0x4f,0x00,},
	{0x4f,0x00,},
	{0x4f,0x00,},
	{0x4d,0x36,},
	{0x4e,0x02,},
	{0x4e,0x02,},
	{0x4d,0x44,},
	{0x4e,0x02,},
	{0x4e,0x02,},
	{0x4e,0x02,},
	{0x4e,0x02,},
	{0x4d,0x53,},
	{0x4e,0x08,},
	{0x4e,0x08,},
	{0x4e,0x02,},
	{0x4d,0x63,},
	{0x4e,0x08,},
	{0x4e,0x08,},
	{0x4d,0x73,},
	{0x4e,0x20,},
	{0x4d,0x83,},
	{0x4e,0x20,},
	{0x4f,0x01,},
	{0x50,0x88,},
	{0xfe,0x00,},
	////////////////////////////////////////////////
	////////////     BLK      //////////////////////
	////////////////////////////////////////////////
	{0x27,0x00,},
	{0x2a,0x40,},
	{0x2b,0x40,},
	{0x2c,0x40,},
	{0x2d,0x40,},
	//////////////////////////////////////////////
	////////// page  0    ////////////////////////
	//////////////////////////////////////////////
	{0xfe,0x00,},
	{0x09,0x00,},
	{0x0a,0x00,},
	{0x0b,0x00,},
	{0x0c,0x00,},
	{0x0d,0x01,},
	{0x0e,0xe8,},
	{0x0f,0x02,},
	{0x10,0x88,},
	{0x16,0x00,},
	{0x17,0x14,},
	{0x18,0x0e,},
	{0x19,0x06,},
	{0x1b,0x48,},
	{0x1f,0xC8,},
	{0x20,0x01},
	{0x21,0x78},
	{0x22,0xb0},
	{0x23,0x04},
	{0x24,0x11},
	{0x26,0x00},
	{0x50,0x01},
	{0x70,0x85,},
	////////////////////////////////////////////////
	////////////     block enable      /////////////
	////////////////////////////////////////////////
	{0x40,0x7f,},
	{0x41,0x26,},
	{0x42,0xff},
	{0x45,0x00,},
	{0x44,0x01,},//
	{0x46,0x02,},
	{0x4b,0x01,},
	{0x50,0x01,},
	/////DN & EEINTP/////
	{0x7e,0x0a,},
	{0x7f,0x03,},
	{0x80,0x27,},
	{0x81,0x15,},
	{0x82,0x90,},
	{0x83,0x02,},
	{0x84,0x23,},
	{0x90,0x2c,},
	{0x92,0x02,},
	{0x94,0x02,},
	{0x95,0x35,},
	///////YCP
	{0xd1,0x32,},
	{0xd2,0x32,},
	{0xdd,0x18,},
	{0xde,0x32,},
	{0xe4,0x88,},
	{0xe5,0x40,},
	{0xd7,0x0e,},
	/////////////////////////////
	////////////////}, GAMMA //////},
	/////////////////////////////
	//},rgb gamma},
	{0xfe,0x00,},
	{0xbf,0x10,},
	{0xc0,0x1c,},
	{0xc1,0x33,},
	{0xc2,0x48,},
	{0xc3,0x5a,},
	{0xc4,0x6b,},
	{0xc5,0x7b,},
	{0xc6,0x95,},
	{0xc7,0xab,},
	{0xc8,0xbf,},
	{0xc9,0xcd,},
	{0xca,0xd9,},
	{0xcb,0xe3,},
	{0xcc,0xeb,},
	{0xcd,0xf7,},
	{0xce,0xfd,},
	{0xcf,0xff,},
	/////////////Y gamma},//////////
	{0xfe,0x00,},
	{0x63,0x00,},
	{0x64,0x05,},
	{0x65,0x0c,},
	{0x66,0x1a,},
	{0x67,0x29,},
	{0x68,0x39,},
	{0x69,0x4b,},
	{0x6a,0x5e,},
	{0x6b,0x82,},
	{0x6c,0xa4,},
	{0x6d,0xc5,},
	{0x6e,0xe5,},
	{0x6f,0xFF,},
	//////ASDE             },
	{0xfe,0x01,},
	{0x18,0x02,},
	{0xfe,0x00,},
	{0x98,0x00,},
	{0x9b,0x20,},
	{0x9c,0x80,},
	{0xa4,0x10,},
	{0xa8,0xB0,},
	{0xaa,0x40,},
	{0xa2,0x23,},
	{0xad,0x01,},
	//////////////////////////////////////////////
	////////// AEC},    ////////////////////////},
	//////////////////////////////////////////////
	{0xfe,0x01},
	{0x9c,0x02,},
	{0x08,0xa0,},
	{0x09,0xe8,},
	{0x10,0x00,},
	{0x11,0x11,},
	{0x12,0x10,},
	{0x13,0x80,},
	{0x15,0xfc,},
	{0x18,0x03,},
	{0x21,0xc0,},
	{0x22,0x60,},
	{0x23,0x30,},
	{0x25,0x00,},
	{0x24,0x14,},
	//////////////////////////////////////
	////////////LSC},//////////////////////},
	//////////////////////////////////////
	{0xfe,0x01},
	{0xc0,0x10,},
	{0xc1,0x0c,},
	{0xc2,0x0a,},
	{0xc6,0x0e,},
	{0xc7,0x0b,},
	{0xc8,0x0a,},
	{0xba,0x26,},
	{0xbb,0x1c,},
	{0xbc,0x1d,},
	{0xb4,0x23,},
	{0xb5,0x1c,},
	{0xb6,0x1a,},
	{0xc3,0x00,},
	{0xc4,0x00,},
	{0xc5,0x00,},
	{0xc9,0x00,},
	{0xca,0x00,},
	{0xcb,0x00,},
	{0xbd,0x00,},
	{0xbe,0x00,},
	{0xbf,0x00,},
	{0xb7,0x07,},
	{0xb8,0x05,},
	{0xb9,0x05,},
	{0xa8,0x07,},
	{0xa9,0x06,},
	{0xaa,0x00,},
	{0xab,0x04,},
	{0xac,0x00,},
	{0xad,0x02,},
	{0xae,0x0d,},
	{0xaf,0x05,},
	{0xb0,0x00,},
	{0xb1,0x07,},
	{0xb2,0x03,},
	{0xb3,0x00,},
	{0xa4,0x00,},
	{0xa5,0x00,},
	{0xa6,0x00,},
	{0xa7,0x00,},
	{0xa1,0x3c,},
	{0xa2,0x50,},
	{0xfe,0x00,},
	///////////////CCT ///////////},
	{0xB1,0x04,},
	{0xB2,0xfd,},
	{0xB3,0xfc,},
	{0xB4,0xf0,},
	{0xB5,0x05,},
	{0xB6,0xf0,},
	{0xfe,0x00,},
	{0x27,0xf7,},
	{0x28,0x7F,},
	{0x29,0x20,},
	{0x33,0x20,},
	{0x34,0x20,},
	{0x35,0x20,},
	{0x36,0x20,},
	{0x32,0x08,},
	{0x47,0x00,},
	{0x48,0x00,},
	{0xfe,0x01,},
	{0x79,0x00,},
	{0x7d,0x00,},
	{0x50,0x88,},
	{0x5b,0x0c,},
	{0x76,0x8f,},
	{0x80,0x70,},
	{0x81,0x70,},
	{0x82,0xb0,},
	{0x70,0xff,},
	{0x71,0x00,},
	{0x72,0x28,},
	{0x73,0x0b,},
	{0x74,0x0b,},
	{0xfe,0x00,},
	{0x70,0x45,},
	{0x4f,0x01,},
	{0xf1,0x07,},
	{0xf2,0x01,},
};

static int gc0328_write_reg(const struct device *dev, uint8_t reg_addr, uint8_t value)
{
	struct gc0328_data *drv_data = dev->data;
	struct i2c_msg msgs;
	uint8_t tx_buf[2];

	tx_buf[0] = reg_addr;
	tx_buf[1] = value;

	msgs.buf = tx_buf;
	msgs.len = 2;
	msgs.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(drv_data->i2c_bus.bus, &msgs, 1, drv_data->i2c_bus.addr);
}

static int gc0328_read_reg(const struct device *dev, uint8_t reg_addr, uint8_t *value)
{
	struct gc0328_data *drv_data = dev->data;
	struct i2c_msg msgs[2];

	msgs[0].buf = (uint8_t *)&reg_addr;
	msgs[0].len = 1;
	/*
	 * When using I2C to read the registers of the SCCB device,
	 * a stop bit is required after writing the register address
	 */
	msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	msgs[1].buf = (uint8_t *)value;
	msgs[1].len = 1;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP | I2C_MSG_RESTART;

	return i2c_transfer(drv_data->i2c_bus.bus, msgs, 2, drv_data->i2c_bus.addr);
}

int gc0328_modify_reg(const struct device *dev, uint8_t reg_addr, uint8_t clear_mask, uint8_t value)
{
	int ret;
	uint8_t set_value;

	ret = gc0328_read_reg(dev, reg_addr, &set_value);

	if (ret == 0) {
		set_value = (set_value & (~clear_mask)) | (set_value & clear_mask);
		ret = gc0328_write_reg(dev, reg_addr, set_value);
	}

	return ret;
}

static int gc0328_write_all(const struct device *dev, const struct gc0328_reg *regs,
			    uint16_t reg_num)
{
	uint16_t i = 0;
	int err;

	for (i = 0; i < reg_num; i++) {
		err = gc0328_write_reg(dev, regs[i].addr, regs[i].value);
		// k_sleep(K_MSEC(1));
		if (err) {
			return err;
		}
	}

	return 0;
}

static int gc0328_set_fmt_windows(const struct device *dev, int offset_x, int offset_y, int w, int h)
{
	gc0328_write_reg(dev, 0xfe, 0x00);
	gc0328_write_reg(dev, 0x51, (offset_y>>8) & 0xff);
	gc0328_write_reg(dev, 0x52, offset_y & 0xff);
	gc0328_write_reg(dev, 0x53, (offset_x>>8) & 0xff);
	gc0328_write_reg(dev, 0x54, offset_x & 0xff);
	gc0328_write_reg(dev, 0x55, (h>>8) & 0x01);
	gc0328_write_reg(dev, 0x56, h & 0xff);
	gc0328_write_reg(dev, 0x57, (w>>8) & 0x03);
	gc0328_write_reg(dev, 0x58, w & 0xff);
	return 0;
}

static int gc0328_set_fmt(const struct device *dev, enum video_endpoint_id ep,
			    struct video_format *fmt)
{
	uint16_t offset_x, offset_y;

	gc0328_write_reg(dev, 0xfe, 0x00);

	if (fmt->pixelformat == VIDEO_PIX_FMT_VYUY) {
		gc0328_write_reg(dev, 0x44, 0x01);
	} else if (fmt->pixelformat == VIDEO_PIX_FMT_RGB565) {
		gc0328_write_reg(dev, 0x44, 0x06);
	} else {
		LOG_ERR("Unsupported format: %d", fmt->pixelformat);
		return -ENOTSUP;
	}

	if ((fmt->width > 640) || (fmt->height > 480)) {
		LOG_ERR("Unsupported resolution: %d x %d", fmt->width, fmt->height);
		return -ENOTSUP;
	}

	offset_x = (640 - fmt->width) / 2;
	offset_y = (480 - fmt->height) / 2;
	gc0328_set_fmt_windows(dev, offset_x, offset_y, fmt->width, fmt->height);

	return 0;
}

static int gc0328_get_fmt(const struct device *dev, enum video_endpoint_id ep,
			  struct video_format *fmt)
{
	return 0;
}

static int gc0328_stream_start(const struct device *dev)
{
	return 0;
}

static int gc0328_stream_stop(const struct device *dev)
{
	return 0;
}

static const struct video_format_cap fmts[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_VYUY,
		.width_min = 160,
		.width_max = 640,
		.height_min = 120,
		.height_max = 480,
		.width_step = 0,
		.height_step = 0,
	},
	{
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width_min = 160,
		.width_max = 640,
		.height_min = 120,
		.height_max = 480,
		.width_step = 0,
		.height_step = 0,
	},
	{0}
};

static int gc0328_get_caps(const struct device *dev, enum video_endpoint_id ep,
			   struct video_caps *caps)
{
	caps->format_caps = fmts;
	return 0;
}

static int gc0328_set_horizontal_mirror(const struct device *dev, int enable)
{
	int ret = 0;
	uint8_t reg_mirror = 0;
	struct gc0328_data *drv_data = dev->data;

	gc0328_read_reg(dev, PAGE_BANK_REG_CID_MIRROR, &reg_mirror);
	if (enable) {
		reg_mirror |= REG_CID_MIRROR_ON_VALUE;
	} else {
		reg_mirror &= ~REG_CID_MIRROR_ON_VALUE;
	}

	if (drv_data->flip) {
		reg_mirror |= REG_CID_FLIP_ON_VALUE;
	} else {
		reg_mirror &= ~REG_CID_FLIP_ON_VALUE;
	}

	ret = gc0328_write_reg(dev, PAGE_BANK_REG_CID_MIRROR, reg_mirror);

	if (ret) {
		LOG_WRN("Unable to write GC0328 mirror ret=%d", ret);
		return ret;
	}

	drv_data->mirror = enable;

	return ret;
}

static int gc0328_set_vertical_flip(const struct device *dev, int enable)
{
	int ret = 0;
	uint8_t reg_value = 0;
	struct gc0328_data *drv_data = dev->data;

	gc0328_read_reg(dev, PAGE_BANK_REG_CID_FLIP, &reg_value);

	if (enable) {
		reg_value |= REG_CID_FLIP_ON_VALUE;
	} else {
		reg_value &= ~REG_CID_FLIP_ON_VALUE;
	}

	if (drv_data->mirror) {
		reg_value |= REG_CID_MIRROR_ON_VALUE;
	} else {
		reg_value &= ~REG_CID_MIRROR_ON_VALUE;
	}

	ret = gc0328_write_reg(dev, PAGE_BANK_REG_CID_FLIP, reg_value);
	if (ret) {
		LOG_WRN("Unable to write GC0328 flip ret=%d", ret);
		return ret;
	}

	drv_data->flip = enable;

	return ret;
}

static int gc0328_reg_init(const struct device *dev)
{
	struct gc0328_data *drv_data = dev->data;
	int ret = 0;
	uint8_t pid;
	if (i2c_configure(drv_data->i2c_bus.bus,
			  I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER)) {
		LOG_ERR("I2C config failed!");
		return -EIO;
	}

	// check chip id
	ret = gc0328_read_reg(dev, 0xf0, &pid);
	if (ret) {
		LOG_ERR("Unable to read GC0328 pid, ret:%d", ret);
		return -ENODEV;
	}

	if (pid != GC0328_ID) {
		LOG_ERR("GC0328 read PID err 0x%x", pid);
		return -ENODEV;
	}

	LOG_INF("Galaxyc GC0328 init successful, pid 0x%X", pid);

	/* Configure Sensor */
	ret = gc0328_write_all(dev, gc0328_init_reg_tb,
					ARRAY_SIZE(gc0328_init_reg_tb));
	if (ret) {
		LOG_ERR("Unable to write sco30iot config");
		return ret;
	}
	LOG_INF("Galaxyc GC0328 init ok !!!, pid 0x%X", pid);
	return ret;
}

static int gc0328_set_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	LOG_DBG("gc0328 set ctrl %d", cid);
	int ret = 0;

	switch (cid) {
	case VIDEO_CID_HFLIP:
		ret = gc0328_set_horizontal_mirror(dev, *(int *)value);
		break;
	case VIDEO_CID_VFLIP:
		ret = gc0328_set_vertical_flip(dev, *(int *)value);
		break;
	/*
		该sensor只有在mclk有输入时钟的情况下，i2c才能正常通信，所以并不能在内核设备初始化流程中进行sensor初始化。
		目前是通过set_ctrl来提供sensor初始化的api.
	*/
	case VIDEO_CID_INIT:
		k_sleep(K_MSEC(10));
		ret = gc0328_reg_init(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

static int gc0328_init(const struct device *dev)
{
	LOG_INF("Galaxyc GC0328 init");
	struct gc0328_data *drv_data = dev->data;
	int ret = 0;

	if (drv_data->pwdn_gpios.port) {
		gpio_pin_configure_dt(&drv_data->pwdn_gpios, GPIO_OUTPUT_ACTIVE);
		gpio_pin_set_dt(&drv_data->pwdn_gpios, 0);
	}

	if (drv_data->reset_gpios.port) {
		gpio_pin_configure_dt(&drv_data->reset_gpios, GPIO_OUTPUT_ACTIVE);
		gpio_pin_set_dt(&drv_data->reset_gpios, 0);
		k_sleep(K_MSEC(3));
		gpio_pin_set_dt(&drv_data->reset_gpios, 1);
		k_sleep(K_MSEC(1));
	}

	// ret = gc0328_reg_init(dev);
	return ret;
}

static const struct video_driver_api gc0328_driver_api = {
	.set_format = gc0328_set_fmt,
	.get_format = gc0328_get_fmt,
	.get_caps = gc0328_get_caps,
	.stream_start = gc0328_stream_start,
	.stream_stop = gc0328_stream_stop,
	.set_ctrl = gc0328_set_ctrl,
};

#define GC0328_DEVICE_INIT_CSK(index)						\
	static struct gc0328_data gc0328_data_##index = {			\
		.i2c_bus = I2C_DT_SPEC_INST_GET(index),				\
		.reset_gpios = GPIO_DT_SPEC_INST_GET_OR(index, reset_gpios, {}),\
		.pwdn_gpios = GPIO_DT_SPEC_INST_GET_OR(index, pwdn_gpios, {}),	\
	};									\
	DEVICE_DT_INST_DEFINE(index, &gc0328_init, NULL, 			\
				&gc0328_data_##index, NULL,			\
				POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,\
				&gc0328_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GC0328_DEVICE_INIT_CSK)

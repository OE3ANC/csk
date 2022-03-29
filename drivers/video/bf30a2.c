/*
 * Copyright (c) 2022, listenai
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT byd_bf30a2

#include <zephyr.h>
#include <device.h>

#include <sys/byteorder.h>

#include <drivers/video.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <drivers/pwm.h>

#include <stdbool.h>
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>

LOG_MODULE_REGISTER(bf30a2);

struct pwm_dt_spec {
    const struct device *pwm;
    const int pin;
    const int flag;
};

struct bf20a2_chip_data {
    const struct gpio_dt_spec pd_gpios;
    const struct gpio_dt_spec pwr_gpios;
	const struct i2c_dt_spec i2c_bus;
    const struct pwm_dt_spec pwm_bus;
    const struct spi_dt_spec spi_bus;
    const uint16_t product_id;
    const int xclk_freq;
};

struct bf30a2_dev_data {
    struct bf20a2_chip_data chip_data;
};

#if DT_NODE_HAS_PROP(DT_DRV_INST(0), csk6_spec) || DT_INST_ON_BUS(0, i2c)

static int bf30a2_chip_xclk_init(const struct device *dev)
{
	struct bf30a2_dev_data *data = (struct bf30a2_dev_data *)dev->data;

	if (!device_is_ready(data->chip_data.pwm_bus.pwm)) {
		LOG_ERR("PWM device is not ready!");
		return -1;
	}

	uint32_t period = 80;
	if (pwm_pin_set_nsec(data->chip_data.pwm_bus.pwm, data->chip_data.pwm_bus.pin, period,
			     period / 2, data->chip_data.pwm_bus.flag)) {
		LOG_ERR("PWM device config error!");
		return -1;
	}

	return 0;
}

static int bf30a2_chip_power_enable(const struct device *dev, uint8_t status)
{
	struct bf30a2_dev_data *data = (struct bf30a2_dev_data *)dev->data;

	return gpio_pin_configure(data->chip_data.pwr_gpios.port, data->chip_data.pwr_gpios.pin,
        (status ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE) | data->chip_data.pwr_gpios.dt_flags);
}

static int bf30a2_chip_power_down_enable(const struct device *dev, uint8_t status)
{
    struct bf30a2_dev_data *data = (struct bf30a2_dev_data *)dev->data;

	return gpio_pin_configure(data->chip_data.pd_gpios.port, data->chip_data.pd_gpios.pin,
        (status ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE) | data->chip_data.pd_gpios.dt_flags);
}

static int bf30a2_chip_write_register(const struct device *dev, uint8_t reg, uint8_t value)
{
	struct bf30a2_dev_data *data = (struct bf30a2_dev_data *)dev->data;
	struct i2c_msg msg = { 0 };
	uint8_t buf[2] = { 0 };

	buf[0] = reg;
	buf[1] = value;

	msg.buf = buf;
	msg.len = sizeof(buf);
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(data->chip_data.i2c_bus.bus, &msg, 1, data->chip_data.i2c_bus.addr);
}

static int bf30a2_chip_read_regsiter(const struct device *dev, uint8_t reg, uint8_t *buf)
{
    struct bf30a2_dev_data *data = (struct bf30a2_dev_data *)dev->data;
    struct i2c_msg msg[2] = {0};

    msg[0].buf = &reg;
    msg[0].len = sizeof(reg);
    msg[0].flags = I2C_MSG_WRITE;

    msg[1].buf = buf;
	msg[1].len = 1;
	msg[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

    return i2c_transfer(data->chip_data.i2c_bus.bus, &msg[0], 2, data->chip_data.i2c_bus.addr);
}

static int bf30a2_chip_i2c_init(const struct device *dev)
{
	struct bf30a2_dev_data *data = (struct bf30a2_dev_data *)dev->data;

	if (!device_is_ready(data->chip_data.i2c_bus.bus)) {
		LOG_ERR("I2C is not ready!");
		return -1;
	}

	if (i2c_configure(data->chip_data.i2c_bus.bus, I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_MASTER)) {
		LOG_ERR("I2C config failed!");
		return -1;
	}

    /* Try to read the product ID */
    uint8_t tmp = 0;
    uint16_t id = 0;
    int32_t r;

    r = bf30a2_chip_read_regsiter(dev, 0xfc, &tmp);
    if (r != 0) {
        return r;
    }
    id = tmp;
    id <<= 8;
    r = bf30a2_chip_read_regsiter(dev, 0xfd, &tmp);
    if (r != 0) {
        return r;
    }
    id |= tmp;
    if (id != data->chip_data.product_id) {
        LOG_ERR("An incorrect product ID(%04X) was read, target product ID(%04x).", id, data->chip_data.product_id);
    }

	return 0;
}

static int bf30a2_chip_config(const struct device *dev)
{

#define X_AXIS 0x3b
#define V_START 0x46
#define V_STOP 0x7d

	const uint8_t regs_config[][2] = {
		{ 0xf2, 0x01 }, ////软复位
		{ 0x2F, X_AXIS }, //	48~176
		//{0x2F, 0x7f},  //	112~240
		// {0x2F, 0x6e},  //	96~224
		{ 0x2e, 0x57 }, //
		{ 0x07, 0x52 }, //
		{ 0x2c, 0x51 }, //
		{ 0x12, 0xc3 }, //
		{ 0x15, 0x80 }, //
		{ 0x6b, 0x71 }, //	71:YUYV	79:YVYU
		{ 0x04, 0x00 }, //	aa:10fps
		{ 0x06, 0x26 }, //
		{ 0x08, 0x07 }, //
		{ 0x1c, 0x12 }, //
		{ 0x1e, 0xae }, //
		{ 0x1f, 0x00 }, //
		{ 0x20, 0x20 }, //
		{ 0x21, 0x20 }, //
		{ 0x34, 0x02 }, //
		{ 0x35, 0x02 }, //
		{ 0x36, 0x21 }, //
		{ 0x37, 0x13 }, //
		{ 0xca, 0x07 }, //
		{ 0xcb, 0x22 }, //
		{ 0xcc, 0x89 }, //
		{ 0xcd, 0x4c }, //
		{ 0xce, 0x6b }, //
		{ 0xcf, 0xf0 }, //
		{ 0xa0, 0x8e }, //
		{ 0x01, 0x1b }, //
		{ 0x02, 0x1d }, //
		{ 0x13, 0x08 }, //
		{ 0x87, 0x13 }, //
		//{0x8a, 0xff},  //
		{ 0x8a, 0x1a }, //	较少曝光时间
		{ 0x8b, 0x08 }, //
		{ 0x70, 0x1d }, //
		{ 0x71, 0x40 }, //
		{ 0x72, 0x02 }, //
		{ 0x73, 0x62 }, //
		{ 0x74, 0x22 }, //
		{ 0x75, 0xbf }, //
		{ 0x76, 0x01 }, //
		{ 0x77, 0xcc }, //
		{ 0x40, 0x32 }, //
		{ 0x41, 0x28 }, //
		{ 0x42, 0x26 }, //
		{ 0x43, 0x1d }, //
		{ 0x44, 0x1a }, //
		{ 0x45, 0x14 }, //
		{ 0x46, 0x11 }, //
		{ 0x47, 0x0f }, //
		{ 0x48, 0x0e }, //
		{ 0x49, 0x0d }, //
		{ 0x4B, 0x0c }, //
		{ 0x4C, 0x0b }, //
		{ 0x4E, 0x0a }, //
		{ 0x4F, 0x09 }, //
		{ 0x50, 0x09 }, //
		{ 0x24, 0x50 }, //
		{ 0x25, 0x36 }, //
		{ 0x80, 0x00 }, //
		{ 0x81, 0x20 }, //
		{ 0x82, 0x40 }, //
		{ 0x83, 0x30 }, //
		{ 0x84, 0x50 }, //
		{ 0x85, 0x30 }, //
		{ 0x86, 0x30 }, //
		{ 0x89, 0x45 }, //
		{ 0x8f, 0x81 }, //
		{ 0x91, 0xff }, //
		{ 0x92, 0x08 }, //
		{ 0x94, 0x82 }, //
		{ 0x95, 0xfd }, //
		{ 0x9a, 0x20 }, //
		{ 0x9e, 0xbc }, //
		{ 0xf0, 0x81 }, //
		{ 0x51, 0x06 }, //
		{ 0x52, 0x25 }, //
		{ 0x53, 0x2b }, //
		{ 0x54, 0x0F }, //
		{ 0x57, 0x2A }, //
		{ 0x58, 0x22 }, //
		{ 0x59, 0x2c }, //
		{ 0x23, 0x33 }, //
		{ 0xa0, 0x8f }, //
		{ 0xa1, 0x93 }, //
		{ 0xa2, 0x0f }, //
		{ 0xa3, 0x2a }, //
		{ 0xa4, 0x08 }, //
		{ 0xa5, 0x26 }, //
		{ 0xa7, 0x80 }, //
		{ 0xa8, 0x80 }, //
		{ 0xa9, 0x1e }, //
		{ 0xaa, 0x19 }, //
		{ 0xab, 0x18 }, //
		{ 0xae, 0x50 }, //
		{ 0xaf, 0x04 }, //
		{ 0xc8, 0x10 }, //
		{ 0xc9, 0x15 }, //
		{ 0xd3, 0x0c }, //
		{ 0xd4, 0x16 }, //
		{ 0xee, 0x06 }, //
		{ 0xef, 0x04 }, //
		{ 0x55, 0x34 }, //
		{ 0x56, 0x9c }, //
		{ 0xb1, 0x98 }, //
		{ 0xb2, 0x98 }, //
		{ 0xb3, 0xc4 }, //
		{ 0xb4, 0x0C }, //
		{ 0x00, 0x42 }, //	00
		//{0x00, 0x46},  //	01
		//{0x00, 0x4A},  //	10
		//{0x00, 0x4e},  //	11
		{ 0x13, 0x07 }, //
		{ 0xf1, 0x09 }, //
		{ 0xa0, 0x8E }, //
		{ 0xa1, 0x93 }, //
		{ 0x23, 0x22 }, //
		{ 0x01, 0x0a }, //
		{ 0x02, 0x0a }, //
		{ 0x17, 0x00 }, //
		{ 0x18, 0x80 }, //
		// {0x19, 0x28},  //
		// {0x1a, 0x6e},  //	110*2
		{ 0x19, V_START }, //	70
		{ 0x1a, V_STOP }, //
	};
    uint16_t i;
    struct bf30a2_dev_data *data = (struct bf30a2_dev_data *)dev->data;

    for (i = 0; i < sizeof(regs_config) / sizeof(regs_config[0]); i++) {
        if (bf30a2_chip_write_register(data->chip_data.i2c_bus.bus, regs_config[i][0], regs_config[i][1]) != 0) {
            LOG_ERR("Set register %x error!", regs_config[i][0]);
            return -1;
        }
    }

	return 0;
}
#endif

#if !DT_NODE_HAS_PROP(DT_DRV_INST(0), csk6_spec) || DT_INST_ON_BUS(0, spi)
static int bf30a2_chip_spi_init(const struct device *dev)
{
    struct bf30a2_dev_data *data = (struct bf30a2_dev_data *)dev->data;

    if (!device_is_ready(data->chip_data.spi_bus.bus)) {
        LOG_ERR("SPI bus is not ready!");
        return -1;
    }

    return spi_transceive_dt(data->chip_data.spi_bus, NULL, NULL);
}
#endif

static int bf30a2_chip_init(const struct device *dev)
{
    int r;

    /** 
     * NOTE THAT: bf30a2 must aways be on bus i2c if the csk6_spec property is not defined.
     */
#if DT_NODE_HAS_PROP(DT_DRV_INST(0), csk6_spec) || DT_INST_ON_BUS(0, i2c)
	r = bf30a2_chip_xclk_init(dev);
    if (r != 0) {
        LOG_ERR("XCLK init error.");
        return r;
    }
	r = bf30a2_chip_power_enable(dev, 1);
    if (r != 0) {
        LOG_ERR("power enable error.");
        return r;
    }
	/* TODO: min interval 100us at here */
    /* k_sleep(); */
	r = bf30a2_chip_power_down_enable(dev, 0);
    if (r != 0) {
        LOG_ERR("power dowm mode disable error.");
        return r;
    }
	/* TODO: min interval 50us at here */
    /* k_sleep(); */
	r = bf30a2_chip_i2c_init(dev);
    if (r != 0) {
        LOG_ERR("i2c init error.");
        return r;
    }
    r = bf30a2_chip_config(dev);
    if (r != 0) {
        LOG_ERR("chip config error.");
        return r;
    }
#endif

#if !DT_NODE_HAS_PROP(DT_DRV_INST(0), csk6_spec) || DT_INST_ON_BUS(0, spi)
	r = bf30a2_chip_spi_init(dev);
    if (r != 0) {
        LOG_ERR("spi init error.");
        return r;
    }
#endif

	return r;
}

static int bf30a2_driv_set_format(const struct device *dev, enum video_endpoint_id ep,
				  struct video_format *fmt)
{
	return 0;
}

static int bf30a2_driv_get_format(const struct device *dev, enum video_endpoint_id ep,
				  struct video_format *fmt)
{
	return 0;
}

static int bf30a2_driv_get_caps(const struct device *dev, enum video_endpoint_id ep,
				struct video_caps *caps)
{
	return 0;
}

static int bf30a2_driv_stream_start(const struct device *dev)
{
	return 0;
}

static int bf30a2_driv_stream_stop(const struct device *dev)
{
	return 0;
}

static const struct video_driver_api bf30a2_driv_api = {
	.set_format = bf30a2_driv_set_format,
	.get_format = bf30a2_driv_get_format,
	.get_caps = bf30a2_driv_get_caps,
	.stream_start = bf30a2_driv_stream_start,
	.stream_stop = bf30a2_driv_stream_stop,
};

static int bf30a2_dev_init(const struct device *dev)
{
	bf30a2_chip_init(dev);
	return 0;
}

#define BF30A2_SPI_CONFIG_DT(node_id, operation_, delay_)			\
	{								                                \
		.frequency = DT_PROP(node_id, spi_max_frequency),	        \
		.operation = (operation_),				                    \
		.slave = 0,				                                    \
		.cs = COND_CODE_1(					                        \
			DT_SPI_DEV_HAS_CS_GPIOS(node_id),		                \
			(SPI_CS_CONTROL_PTR_DT(node_id, delay_)),	            \
			(NULL)),					                            \
	}

#define BF30A2_SPI_DT_SPEC_GET(node_id, operation_, delay_)		     \
	{							     \
		.bus = DEVICE_DT_GET(DT_PROP(node_id, spi)),		     \
		.config = BF30A2_SPI_CONFIG_DT(DT_PROP(node_id, spi), operation_, delay_), \
	}

#define BF30A2_PWM_DT_SPEC_GET(node_id)                             \
    {                                                               \
        .pwm = DEVICE_DT_GET(DT_PROP(node_id, xclk_pwm)),           \
        .pin = DT_PHA_BY_IDX(DT_PROP(node_id, xclk_pwm), pinmuxs, 0, pin),  \
        .flag = DT_PWMS_FLAGS(DT_PROP(node_id, xclk_pwm)),                  \
    }

#define BF30A2_CHIP_DATA(inst)  \
    {                           \
        .pd_gpios = GPIO_DT_SPEC_GET(DT_DRV_INST(inst), pd_gpios),     \
        .pwr_gpios = GPIO_DT_SPEC_GET(DT_DRV_INST(inst), pwr_gpios),   \
        .i2c_bus = I2C_DT_SPEC_GET(DT_DRV_INST(inst)),                 \
        .spi_bus = BF30A2_SPI_DT_SPEC_GET(DT_DRV_INST(inst), SPI_WORD_SET(8) | SPI_OP_MODE_SLAVE | SPI_TRANSFER_MSB, 0),   \
        .pwm_bus = BF30A2_PWM_DT_SPEC_GET(DT_DRV_INST(inst)),          \
        .product_id = DT_PROP(DT_DRV_INST(inst), product_id),          \
        .xclk_freq = DT_PROP(DT_DRV_INST(inst), xclk_freq),            \
    }

static struct bf30a2_dev_data bf30a2_dev_data = {
    .chip_data = BF30A2_CHIP_DATA(0),
};

DEVICE_DT_INST_DEFINE(0, &bf30a2_dev_init, NULL, &bf30a2_dev_data, NULL, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &bf30a2_driv_api);
/*
 * Copyright (c) 2025 Anhui Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT listenai_sc7a20h

#include "sc7a20h.h"

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(SC7A20H_CSK6, CONFIG_LOG_DEFAULT_LEVEL);

static void sc7a20h_convert_accel(struct sensor_value *val, int16_t raw_val, uint16_t sensitivity)
{
	int64_t conv_val;
	conv_val = ((int64_t)raw_val * SENSOR_G) / sensitivity;
	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

static int sc7a20h_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct sc7a20h_data *drv_data = dev->data;

	if (!drv_data) {
		return -EINVAL;
	}

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		sc7a20h_convert_accel(val, drv_data->accel_x, drv_data->sensitivity);
		sc7a20h_convert_accel(val + 1, drv_data->accel_y, drv_data->sensitivity);
		sc7a20h_convert_accel(val + 2, drv_data->accel_z, drv_data->sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_X:
		sc7a20h_convert_accel(val, drv_data->accel_x, drv_data->sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		sc7a20h_convert_accel(val, drv_data->accel_y, drv_data->sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		sc7a20h_convert_accel(val, drv_data->accel_z, drv_data->sensitivity);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int sc7a20h_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct sc7a20h_data *drv_data = dev->data;
	const struct sc7a20h_config *cfg = dev->config;
	uint8_t buf[6];
	int ret;

	uint8_t status;
	ret = i2c_reg_read_byte_dt(&cfg->i2c, SC7A20H_REG_STATUS, &status);
	if (ret < 0 || !(status & 0x08)) {
		LOG_ERR("Data not ready or read failed");
		return -EIO;
	}

	ret = i2c_burst_read_dt(&cfg->i2c, SC7A20H_REG_DATA_START, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to read acceleration data");
		return ret;
	}

	drv_data->accel_x = (int16_t)((buf[1] << 8) | buf[0]);
	drv_data->accel_y = (int16_t)((buf[3] << 8) | buf[2]);
	drv_data->accel_z = (int16_t)((buf[5] << 8) | buf[4]);

	return 0;
}

static const struct sensor_driver_api sc7a20h_driver_api = {
	.sample_fetch = sc7a20h_sample_fetch,
	.channel_get = sc7a20h_channel_get,
#ifdef CONFIG_SC7A20H_TRIGGER
	.trigger_set = sc7a20h_trigger_set,
#endif
};

static int sc7a20h_init(const struct device *dev)
{
	struct sc7a20h_data *drv_data = dev->data;
	const struct sc7a20h_config *cfg = dev->config;
	uint8_t chip_id;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	drv_data->accel_x = 0;
	drv_data->accel_y = 0;
	drv_data->accel_z = 0;
	drv_data->sensitivity = SC7A20H_SENSITIVITY_2G;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, SC7A20H_REG_CHIP_ID, &chip_id);
	if (ret < 0) {
		LOG_ERR("Failed to read chip ID");
		return ret;
	}

	if (chip_id != SC7A20H_CHIP_ID) {
		LOG_ERR("Invalid chip ID: 0x%02x", chip_id);
		return -EINVAL;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, SC7A20H_REG_CTRL2, 0x40);
	if (ret < 0) {
		LOG_ERR("Failed to reset device");
		return ret;
	}
	k_msleep(10);

	ret = i2c_reg_write_byte_dt(&cfg->i2c, SC7A20H_REG_CTRL1,
				    SC7A20H_CTRL1_XEN | SC7A20H_CTRL1_YEN | SC7A20H_CTRL1_ZEN |
					    SC7A20H_CTRL1_ODR_50HZ);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, SC7A20H_REG_CTRL4, SC7A20H_CTRL4_FS_2G | BIT(3));
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_SC7A20H_TRIGGER
	if (cfg->int_gpio.port) {
		ret = sc7a20h_init_interrupt(dev);
		if (ret < 0) {
			LOG_ERR("Failed to initialize interrupts");
			return ret;
		}
	}
#endif

	return 0;
}

#define SC7A20H_DEFINE(inst)                                                                       \
	static struct sc7a20h_data sc7a20h_data_##inst;                                            \
                                                                                                   \
	static const struct sc7a20h_config sc7a20h_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		IF_ENABLED(CONFIG_SC7A20H_TRIGGER,                                                 \
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}), ))};       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, sc7a20h_init, NULL, &sc7a20h_data_##inst,                      \
			      &sc7a20h_config_##inst, POST_KERNEL, 75, &sc7a20h_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SC7A20H_DEFINE)

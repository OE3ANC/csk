/*
 * Copyright (c) 2023 Anhui Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT listenai_qmi8658c

#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "qmi8658c.h"

LOG_MODULE_REGISTER(QMI8658C_CSK6, CONFIG_SENSOR_LOG_LEVEL);

static void qmi8658c_convert_accel(struct sensor_value *val, int16_t raw_val,
                                uint16_t sensitivity_shift)
{
    int64_t conv_val;

    conv_val = ((int64_t)raw_val * SENSOR_G) >> sensitivity_shift;
    val->val1 = conv_val / 1000000;
    val->val2 = conv_val % 1000000;
}

static void qmi8658c_convert_gyro(struct sensor_value *val, int16_t raw_val,
                               uint16_t sensitivity_x10)
{
    int64_t conv_val;

    conv_val = ((int64_t)raw_val * SENSOR_PI * 10) /
               (sensitivity_x10 * 180U);
    val->val1 = conv_val / 1000000;
    val->val2 = conv_val % 1000000;
}

static void qmi8658c_convert_temp(struct sensor_value *val, int16_t raw_val)
{
    /* QMI8658C temperature conversion formula */
    val->val1 = raw_val / 256 + 25;
    val->val2 = (raw_val % 256) * 1000000 / 256;
}

static int qmi8658c_channel_get(const struct device *dev,
                             enum sensor_channel chan,
                             struct sensor_value *val)
{
    struct qmi8658c_data *drv_data = dev->data;

    switch (chan) {
    case SENSOR_CHAN_ACCEL_XYZ:
        qmi8658c_convert_accel(val, drv_data->accel_x,
                            drv_data->accel_sensitivity_shift);
        qmi8658c_convert_accel(val + 1, drv_data->accel_y,
                            drv_data->accel_sensitivity_shift);
        qmi8658c_convert_accel(val + 2, drv_data->accel_z,
                            drv_data->accel_sensitivity_shift);
        break;
    case SENSOR_CHAN_GYRO_XYZ:
        qmi8658c_convert_gyro(val, drv_data->gyro_x,
                           drv_data->gyro_sensitivity_x10);
        qmi8658c_convert_gyro(val + 1, drv_data->gyro_y,
                           drv_data->gyro_sensitivity_x10);
        qmi8658c_convert_gyro(val + 2, drv_data->gyro_z,
                           drv_data->gyro_sensitivity_x10);
        break;
    case SENSOR_CHAN_DIE_TEMP:
        qmi8658c_convert_temp(val, drv_data->temp);
        break;
    default:
        return -ENOTSUP;
    }

    return 0;
}

static int qmi8658c_sample_fetch(const struct device *dev,
                              enum sensor_channel chan)
{
    struct qmi8658c_data *drv_data = dev->data;
    const struct qmi8658c_config *cfg = dev->config;
    uint8_t status;
    uint8_t buf[12];

    /* 检查数据就绪状态 */
    int ret_data = i2c_reg_read_byte_dt(&cfg->i2c, QMI8658C_STATUS0, &status);
    if (ret_data < 0 || !(status & 0x03)) {
        LOG_ERR("Data not ready or read failed");
        return -EIO;
    }

    /* 读取传感器数据 */
    int ret_sensor = i2c_burst_read_dt(&cfg->i2c, QMI8658C_AX_L, buf, sizeof(buf));
    if (ret_sensor < 0) {
        LOG_ERR("Failed to read sensor data");
        return ret_sensor;
    }

    /* 转换数据 */
    drv_data->accel_x = sys_le16_to_cpu((buf[1] << 8) | buf[0]);
    drv_data->accel_y = sys_le16_to_cpu((buf[3] << 8) | buf[2]);
    drv_data->accel_z = sys_le16_to_cpu((buf[5] << 8) | buf[4]);
    drv_data->gyro_x = sys_le16_to_cpu((buf[7] << 8) | buf[6]);
    drv_data->gyro_y = sys_le16_to_cpu((buf[9] << 8) | buf[8]);
    drv_data->gyro_z = sys_le16_to_cpu((buf[11] << 8) | buf[10]);

    /* 读取温度数据 */
    uint8_t temp[2];
    int ret_temp = i2c_burst_read_dt(&cfg->i2c, QMI8658C_TEMP_L, temp, 2);
    if (ret_temp < 0) {
        LOG_ERR("Failed to read temperature");
        return ret_temp;
    }
    drv_data->temp = sys_le16_to_cpu((temp[1] << 8) | temp[0]);

    return 0;
}

static int qmi8658c_init(const struct device *dev)
{
    const struct qmi8658c_config *cfg = dev->config;
    struct qmi8658c_data *drv_data = dev->data;
    uint8_t chip_id;
    int ret;

    if (!device_is_ready(cfg->i2c.bus)) {
        LOG_ERR("Bus device is not ready");
        return -ENODEV;
    }

    /* 软复位设备 */
    ret = i2c_reg_write_byte_dt(&cfg->i2c, 0x60, 0xB6);
    if (ret < 0) {
        LOG_ERR("Failed to reset device");
        return ret;
    }
    k_msleep(50);  // 等待复位完成

    /* 检查芯片 ID */
    ret = i2c_reg_read_byte_dt(&cfg->i2c, QMI8658C_WHO_AM_I, &chip_id);
    if (ret < 0 || chip_id != QMI8658C_CHIP_ID) {
        LOG_ERR("Failed to read chip ID or wrong ID: %02x", chip_id);
        return -EIO;
    }

    /* 配置 Ctrl1: 使能中断和基本设置 */
    ret = i2c_reg_write_byte_dt(&cfg->i2c, QMI8658C_CTRL1, 
                               0x60 | QMI8658C_INT2_ENABLE | QMI8658C_INT1_ENABLE);
    if (ret < 0) return ret;

    /* 配置加速度计: ±8g, 500Hz */
    ret = i2c_reg_write_byte_dt(&cfg->i2c, QMI8658C_CTRL2, 
                               QMI8658C_ACCRANGE_8G | QMI8658C_ODR_500HZ);
    if (ret < 0) return ret;

    /* 配置陀螺仪: ±512dps, 500Hz */
    ret = i2c_reg_write_byte_dt(&cfg->i2c, QMI8658C_CTRL3, 
                               QMI8658C_GYRORANGE_512DPS | QMI8658C_ODR_500HZ);
    if (ret < 0) return ret;

    /* 配置低通滤波器 */
    ret = i2c_reg_write_byte_dt(&cfg->i2c, QMI8658C_CTRL5, 0x03);
    if (ret < 0) return ret;

    /* 配置传感器使能 */
    ret = i2c_reg_write_byte_dt(&cfg->i2c, QMI8658C_CTRL7,
                               QMI8658C_ACC_ENABLE | QMI8658C_GYR_ENABLE);
    if (ret < 0) return ret;

    /* 设置灵敏度 */
    drv_data->accel_sensitivity_shift = 12;  // ±8g
    drv_data->gyro_sensitivity_x10 = 64;     // ±512dps

    LOG_DBG("QMI8658C initialized successfully");

#ifdef CONFIG_QMI8658C_TRIGGER
    if (cfg->int_gpio.port) {
        ret = qmi8658c_init_interrupt(dev);
        if (ret < 0) {
            LOG_ERR("Failed to initialize interrupts");
            return ret;
        }
    }
#endif

    return 0;
}

static const struct sensor_driver_api qmi8658c_driver_api = {
    .sample_fetch = qmi8658c_sample_fetch,
    .channel_get = qmi8658c_channel_get,
#ifdef CONFIG_QMI8658C_TRIGGER
    .trigger_set = qmi8658c_trigger_set,
#endif
};

#define QMI8658C_DEFINE(inst)                                                    \
    static struct qmi8658c_data qmi8658c_data_##inst;                          \
                                                                               \
    static const struct qmi8658c_config qmi8658c_config_##inst = {            \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                    \
        IF_ENABLED(CONFIG_QMI8658C_TRIGGER,                                   \
                  (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios,      \
                                                      { 0 }),))                \
    };                                                                         \
                                                                               \
    SENSOR_DEVICE_DT_INST_DEFINE(inst, qmi8658c_init, NULL,                  \
                              &qmi8658c_data_##inst,                          \
                              &qmi8658c_config_##inst, POST_KERNEL,           \
                              CONFIG_SENSOR_INIT_PRIORITY,                     \
                              &qmi8658c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(QMI8658C_DEFINE)

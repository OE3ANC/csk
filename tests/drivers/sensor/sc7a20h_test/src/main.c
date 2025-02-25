/*
 * Copyright (c) 2023 Listenai
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <math.h>

// 修改阈值定义
#define ZERO_THRESHOLD 0.2f    // 增大零点阈值
#define GRAVITY_THRESHOLD 0.5f // 重力检测阈值
#define CALIBRATION_SAMPLES 20 // 增加校准样本数
#define FILTER_SAMPLES 5      // 添加滤波样本数定义

// 添加转换因子定义
#define SC7A20H_SENSITIVITY 1024  // 灵敏度: ±2g量程下为1024 LSB/g

// 添加零点偏移存储
static struct {
    float x_offset;
    float y_offset;
    float z_offset;
} zero_offsets = {0};

// 添加滤波缓冲区
static struct {
    float x[FILTER_SAMPLES];
    float y[FILTER_SAMPLES];
    float z[FILTER_SAMPLES];
    uint8_t index;
} filter_buffer = {0};

#define SLEEP_TIME_MS   1000
#define SC7A20H_I2C_ADDR 0x18
#define SC7A20H_WHO_AM_I_REG 0x0F
#define SC7A20H_WHO_AM_I_VAL 0x11 //CHIP_ID

static inline float out_ev(struct sensor_value *val)
{
    return (float)val->val1 + (float)val->val2 / 1000000;
}

static inline float sensor_value_to_ms2(const struct sensor_value *val)
{
    // 将传感器值转换为 m/s²
    // SC7A20H输出的是以重力加速度(g)为单位的值  需要将其转换为m/s²
    
    float g_value = (float)val->val1 + (float)val->val2 / 1000000.0f;
    return g_value * 9.80665f;  // 转换为m/s²
}

static int test_i2c_connection(const struct device *i2c_dev)
{
    uint8_t who_am_i;
    int ret;

    /* 测试I2C通信 */
    ret = i2c_reg_read_byte(i2c_dev, SC7A20H_I2C_ADDR, SC7A20H_WHO_AM_I_REG, &who_am_i);
    if (ret != 0) {
        printk("Failed to read WHO_AM_I register (0x%x)\n", ret);
        return -1;
    }

    if (who_am_i != SC7A20H_WHO_AM_I_VAL) {
        printk("Wrong WHO_AM_I value: 0x%x (expected 0x%x)\n", 
               who_am_i, SC7A20H_WHO_AM_I_VAL);
        return -1;
    }

    printk("I2C connection test passed! WHO_AM_I = 0x%x\n", who_am_i);
    return 0;
}

// 添加校准函数
static void calibrate_sensor(const struct device *dev) {
    struct sensor_value accel[3];
    float x_sum = 0, y_sum = 0, z_sum = 0;
    
    printk("开始校准，请保持传感器静止...\n");
    
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        sensor_sample_fetch(dev);
        sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
        
        x_sum += sensor_value_to_double(&accel[0]);
        y_sum += sensor_value_to_double(&accel[1]);
        z_sum += sensor_value_to_double(&accel[2]);
        
        k_sleep(K_MSEC(100));
    }
    
    zero_offsets.x_offset = x_sum / CALIBRATION_SAMPLES;
    zero_offsets.y_offset = y_sum / CALIBRATION_SAMPLES;
    zero_offsets.z_offset = z_sum / CALIBRATION_SAMPLES;
    
    printk("校准完成，偏移值: X=%f, Y=%f, Z=%f\n",
           zero_offsets.x_offset, zero_offsets.y_offset, zero_offsets.z_offset);
}

// 添加均值滤波函数
static float apply_filter(float new_value, float *buffer) {
    buffer[filter_buffer.index] = new_value;
    float sum = 0;
    
    for (int i = 0; i < FILTER_SAMPLES; i++) {
        sum += buffer[i];
    }
    
    filter_buffer.index = (filter_buffer.index + 1) % FILTER_SAMPLES;
    return sum / FILTER_SAMPLES;
}

// 修改静止状态检测函数
static bool  dev_is_still(float x, float y, float z) {
    float gravity = sqrtf(x*x + y*y + z*z);
    return (fabs(x) < ZERO_THRESHOLD && 
            fabs(y) < ZERO_THRESHOLD && 
            fabs(gravity - 9.81f) < GRAVITY_THRESHOLD);
}

int main(void)
{
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(sc7a20h_csk6_test));
    const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
    uint32_t cnt = 0;
    int ret;

    printk("SC7A20H Sample start\n");

    if (!device_is_ready(i2c_dev)) {
        printk("I2C bus not ready\n");
        return -1;
    }

    if (!device_is_ready(dev)) {
        printk("SC7A20H device not found\n");
        return -1;
    }

    /* 执行I2C通信测试 */
    if (test_i2c_connection(i2c_dev) != 0) {
        printk("I2C communication test failed!\n");
        return -1;
    }

    /* 在开始测试前进行校准 */
    calibrate_sensor(dev);
    
    while (1) {
        struct sensor_value accel[3];
        cnt++;

        printk("\n----- SC7A20H TEST Round %d -----\n", cnt);

        /* 获取传感器数据 */
        ret = sensor_sample_fetch(dev);
        if (ret < 0) {
            printk("sensor_sample_fetch failed ret %d\n", ret);
            continue;
        }

        ret = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
        if (ret < 0) {
            printk("sensor_channel_get failed ret %d\n", ret);
            continue;
        }

        /* 获取数据后应用校准和滤波 */
        float x = sensor_value_to_ms2(&accel[0]) - zero_offsets.x_offset;
        float y = sensor_value_to_ms2(&accel[1]) - zero_offsets.y_offset;
        float z = sensor_value_to_ms2(&accel[2]) - zero_offsets.z_offset;

        /* 应用滤波 */
        x = apply_filter(x, filter_buffer.x);
        y = apply_filter(y, filter_buffer.y);
        z = apply_filter(z, filter_buffer.z);

        /* 应用零点阈值 */
        x = (fabs(x) < ZERO_THRESHOLD) ? 0.0f : x;
        y = (fabs(y) < ZERO_THRESHOLD) ? 0.0f : y;
        z = (fabs(z) < ZERO_THRESHOLD) ? 0.0f : z;

        printk("ACC (m/s^2) after calibration: X=%f, Y=%f, Z=%f\n", x, y, z);

        /* 数据有效性检查 */
        uint8_t check = 1;
        if (!dev_is_still(x, y, z)) {
            check = (fabs(x) <= 16.0f && fabs(y) <= 16.0f && fabs(z) <= 16.0f);
        }

        /* 打印测试结果 */
        if (check) {
            printk("SC7A20H CHECK SUCCESS, %d\n", cnt);
        } else {
            printk("SC7A20H CHECK FAILED\n");
        }

        k_msleep(SLEEP_TIME_MS);
    }
    return 0;
}



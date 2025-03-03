/*
 * Copyright (c) 2025 Anhui Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <math.h>

#define SLEEP_TIME_MS   1000
#define QMI8658C_I2C_ADDR 0x6B
#define QMI8658C_WHO_AM_I_REG 0x00
#define QMI8658C_WHO_AM_I_VAL 0x05 // CHIP_ID
#define QMI8658C_CTRL1       0x02    // 控制寄存器1
#define QMI8658C_CTRL7       0x08    // 控制寄存器7
#define QMI8658C_CTRL8       0x09    // 控制寄存器8

// static int test_i2c_connection(const struct device *i2c_dev)
// {
//     uint8_t who_am_i;
//     int ret;

//     /* 测试I2C通信 */
//     ret = i2c_reg_read_byte(i2c_dev, QMI8658C_I2C_ADDR, QMI8658C_WHO_AM_I_REG, &who_am_i);
//     if (ret != 0) {
//         printk("Failed to read WHO_AM_I register (0x%x)\n", ret);
//         return -1;
//     }

//     if (who_am_i != QMI8658C_WHO_AM_I_VAL) {
//         printk("Wrong WHO_AM_I value: 0x%x (expected 0x%x)\n", 
//                who_am_i, QMI8658C_WHO_AM_I_VAL);
//         return -1;
//     }

//     printk("I2C connection test passed! WHO_AM_I = 0x%x\n", who_am_i);
//     return 0;
// }

// static int configure_sensor(const struct device *i2c_dev)
// {
//     int ret;

//     // 复位传感器
//     ret = i2c_reg_write_byte(i2c_dev, QMI8658C_I2C_ADDR, 0x60, 0xB6);
//     if (ret < 0) {
//         printk("Failed to reset sensor: %d\n", ret);
//         return ret;
//     }
//     k_msleep(50);  // 等待复位完成

//     // 配置 CTRL1: 使能加速度和陀螺仪
//     ret = i2c_reg_write_byte(i2c_dev, QMI8658C_I2C_ADDR, QMI8658C_CTRL1, 0x03);
//     if (ret < 0) {
//         printk("Failed to configure CTRL1: %d\n", ret);
//         return ret;
//     }

//     // 配置 CTRL7: 设置加速度量程(±2g)和ODR(500Hz)
//     ret = i2c_reg_write_byte(i2c_dev, QMI8658C_I2C_ADDR, QMI8658C_CTRL7, 0x03);
//     if (ret < 0) {
//         printk("Failed to configure CTRL7: %d\n", ret);
//         return ret;
//     }

//     // 配置 CTRL8: 设置陀螺仪量程(±250dps)和ODR(500Hz)
//     ret = i2c_reg_write_byte(i2c_dev, QMI8658C_I2C_ADDR, QMI8658C_CTRL8, 0x03);
//     if (ret < 0) {
//         printk("Failed to configure CTRL8: %d\n", ret);
//         return ret;
//     }

//     printk("Sensor configuration completed successfully\n");
//     return 0;
// }

static int process_qmi8658c(const struct device *dev)
{
    struct sensor_value accel[3], gyro[3], temperature;
    int rc;

    rc = sensor_sample_fetch(dev);
    if (rc != 0) {
        printk("sensor_sample_fetch failed: %d\n", rc);
        return rc;
    }

    rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    if (rc != 0) {
        printk("Failed to get acceleration data: %d\n", rc);
        return rc;
    }

    rc = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
    if (rc != 0) {
        printk("Failed to get gyro data: %d\n", rc);
        return rc;
    }

    rc = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temperature);
    if (rc != 0) {
        printk("Failed to get temperature data: %d\n", rc);
        return rc;
    }

    printk("\nTemperature: %.2f °C\n", 
           (double)temperature.val1 + (double)temperature.val2 / 1000000.0);
    printk("Accel (m/s^2): X=%.2f, Y=%.2f, Z=%.2f\n",
           (double)accel[0].val1 + (double)accel[0].val2 / 1000000.0,
           (double)accel[1].val1 + (double)accel[1].val2 / 1000000.0,
           (double)accel[2].val1 + (double)accel[2].val2 / 1000000.0);
    printk("Gyro (rad/s): X=%.2f, Y=%.2f, Z=%.2f\n",
           (double)gyro[0].val1 + (double)gyro[0].val2 / 1000000.0,
           (double)gyro[1].val1 + (double)gyro[1].val2 / 1000000.0,
           (double)gyro[2].val1 + (double)gyro[2].val2 / 1000000.0);

    return 0;
}

static void trigger_handler(const struct device *dev,
                          const struct sensor_trigger *trigger)
{
    int ret;
    struct sensor_value accel[3], gyro[3], temperature;

   
    ret = sensor_sample_fetch(dev);
    if (ret != 0) {
        printk("sensor_sample_fetch failed: %d\n", ret);
        return;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    if (ret == 0) {
        printk("Accel (m/s^2): X=%.2f, Y=%.2f, Z=%.2f\n",
               sensor_value_to_double(&accel[0]),
               sensor_value_to_double(&accel[1]),
               sensor_value_to_double(&accel[2]));
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
    if (ret == 0) {
        printk("Gyro (rad/s): X=%.2f, Y=%.2f, Z=%.2f\n",
               sensor_value_to_double(&gyro[0]),
               sensor_value_to_double(&gyro[1]),
               sensor_value_to_double(&gyro[2]));
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temperature);
    if (ret == 0) {
        printk("Temperature: %.2f °C\n",
               sensor_value_to_double(&temperature));
    }
}

int main(void)
{
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(qmi8658c));
    const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
    uint32_t cnt = 0;
    int ret;

    printk("QMI8658C INT test start\n");

    if (!device_is_ready(i2c_dev)) {
        printk("I2C bus not ready\n");
        return -1;
    }

    if (!device_is_ready(dev)) {
        printk("QMI8658C device not found\n");
        return -1;
    }

    // /* 执行I2C通信测试 */
    // if (test_i2c_connection(i2c_dev) != 0) {
    //     printk("I2C communication test failed!\n");
    //     return -1;
    // }

    // /* 配置传感器 */
    // if (configure_sensor(i2c_dev) != 0) {
    //     printk("Sensor configuration failed!\n");
    //     return -1;
    // }

    /* 配置中断触发 */
    struct sensor_trigger trigger = {
        .type = SENSOR_TRIG_DATA_READY,
        .chan = SENSOR_CHAN_ALL,
    };

    if (sensor_trigger_set(dev, &trigger, trigger_handler) < 0) {
        printk("Cannot configure trigger\n");
        return -1;
    }

    printk("Configured for triggered sampling.\n");
    printk("Move the sensor to generate interrupts!\n");

    while (1) {
        k_sleep(K_MSEC(1000)); /* 仅用于保持主循环运行 */
    }

    return 0;
}

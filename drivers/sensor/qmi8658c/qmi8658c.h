/*
 * Copyright (c) 2025 Anhui Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_QMI8658C_QMI8658C_H_
#define ZEPHYR_DRIVERS_SENSOR_QMI8658C_QMI8658C_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

/* QMI8658C Register Map */
#define QMI8658C_WHO_AM_I    0x00
#define QMI8658C_REVISION_ID 0x01

/* 状态寄存器 */
#define QMI8658C_STATUS0   0x2E
#define QMI8658C_STATUS1   0x2F
#define QMI8658C_STATUSINT 0x2D

#define QMI8658C_CTRL1 0x02
#define QMI8658C_CTRL2 0x03
#define QMI8658C_CTRL3 0x04
#define QMI8658C_CTRL4 0x05
#define QMI8658C_CTRL5 0x06
#define QMI8658C_CTRL7 0x08
#define QMI8658C_CTRL8 0x09

#define QMI8658C_TEMP_L 0x33
#define QMI8658C_TEMP_H 0x34

#define QMI8658C_AX_L 0x35
#define QMI8658C_AX_H 0x36
#define QMI8658C_AY_L 0x37
#define QMI8658C_AY_H 0x38
#define QMI8658C_AZ_L 0x39
#define QMI8658C_AZ_H 0x3A

#define QMI8658C_GX_L 0x3B
#define QMI8658C_GX_H 0x3C
#define QMI8658C_GY_L 0x3D
#define QMI8658C_GY_H 0x3E
#define QMI8658C_GZ_L 0x3F
#define QMI8658C_GZ_H 0x40

#define QMI8658C_CHIP_ID 0x05

#define QMI8658C_ACC_EN	 BIT(0)
#define QMI8658C_GYRO_EN BIT(1)
#define QMI8658C_TEMP_EN BIT(2)

#define QMI8658C_DISABLE_ALL 0x00
#define QMI8658C_ACC_ENABLE  BIT(0)
#define QMI8658C_GYR_ENABLE  BIT(1)

#define QMI8658C_STATUS1_CMD_DONE (0x01)
#define QMI8658C_INT1_ENABLE	  0x08
#define QMI8658C_INT2_ENABLE	  0x10
#define QMI8658C_DRDY_DISABLE	  0x20

#define QMI8658C_ACCRANGE_2G  (0x00 << 4)
#define QMI8658C_ACCRANGE_4G  (0x01 << 4)
#define QMI8658C_ACCRANGE_8G  (0x02 << 4)
#define QMI8658C_ACCRANGE_16G (0x03 << 4)

#define QMI8658C_GYRORANGE_16DPS   (0x00 << 4)
#define QMI8658C_GYRORANGE_32DPS   (0x01 << 4)
#define QMI8658C_GYRORANGE_64DPS   (0x02 << 4)
#define QMI8658C_GYRORANGE_128DPS  (0x03 << 4)
#define QMI8658C_GYRORANGE_256DPS  (0x04 << 4)
#define QMI8658C_GYRORANGE_512DPS  (0x05 << 4)
#define QMI8658C_GYRORANGE_1024DPS (0x06 << 4)
#define QMI8658C_GYRORANGE_2048DPS (0x07 << 4)

enum qmi8658c_odr {
	QMI8658C_ODR_8000HZ = 0x00,
	QMI8658C_ODR_4000HZ = 0x01,
	QMI8658C_ODR_2000HZ = 0x02,
	QMI8658C_ODR_1000HZ = 0x03,
	QMI8658C_ODR_500HZ = 0x04,
	QMI8658C_ODR_250HZ = 0x05,
	QMI8658C_ODR_125HZ = 0x06,
	QMI8658C_ODR_62_5HZ = 0x07,
	QMI8658C_ODR_31_25HZ = 0x08
};

#define QMI8658C_ACCEL_DATA_RDY BIT(0)
#define QMI8658C_GYRO_DATA_RDY	BIT(1)
#define QMI8658C_TEMP_DATA_RDY	BIT(2)

struct qmi8658c_data {
	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;
	uint16_t accel_sensitivity_shift;

	int16_t temp;

	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
	uint16_t gyro_sensitivity_x10;

#ifdef CONFIG_QMI8658C_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t data_ready_handler;
	const struct sensor_trigger *data_ready_trigger;

#if defined(CONFIG_QMI8658C_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_QMI8658C_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_QMI8658C_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_QMI8658C_TRIGGER */
};

struct qmi8658c_config {
	struct i2c_dt_spec i2c;
#ifdef CONFIG_QMI8658C_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
};

#ifdef CONFIG_QMI8658C_TRIGGER
int qmi8658c_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int qmi8658c_init_interrupt(const struct device *dev);
#endif

#endif

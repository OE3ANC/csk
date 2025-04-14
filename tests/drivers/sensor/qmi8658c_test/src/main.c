/*
 * Copyright (c) 2025 Anhui Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define SLEEP_TIME_MS	      1000
#define QMI8658C_I2C_ADDR     0x6B
#define QMI8658C_WHO_AM_I_REG 0x00
#define QMI8658C_WHO_AM_I_VAL 0x05
#define QMI8658C_CTRL1	      0x02
#define QMI8658C_CTRL7	      0x08
#define QMI8658C_CTRL8	      0x09


static void trigger_handler(const struct device *dev, const struct sensor_trigger *trigger)
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
		printk("Accel (m/s^2): X=%.2f, Y=%.2f, Z=%.2f\n", sensor_value_to_double(&accel[0]),
		       sensor_value_to_double(&accel[1]), sensor_value_to_double(&accel[2]));
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
	if (ret == 0) {
		printk("Gyro (rad/s): X=%.2f, Y=%.2f, Z=%.2f\n", sensor_value_to_double(&gyro[0]),
		       sensor_value_to_double(&gyro[1]), sensor_value_to_double(&gyro[2]));
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temperature);
	if (ret == 0) {
		printk("Temperature: %.2f °C\n", sensor_value_to_double(&temperature));
	}
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(qmi8658c));
	const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

	printk("QMI8658C INT test start\n");

	if (!device_is_ready(i2c_dev)) {
		printk("I2C bus not ready\n");
		return -1;
	}

	if (!device_is_ready(dev)) {
		printk("QMI8658C device not found\n");
		return -1;
	}

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
		k_sleep(K_MSEC(1000));
	}

	return 0;
}

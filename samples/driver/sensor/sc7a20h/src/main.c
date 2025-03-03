/*
 * Copyright (c) 2023 Listenai
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>

#define SLEEP_TIME_MS   1000

static inline float sc7a20h_value_to_ms2(const struct sensor_value *val)
{
    return (float)val->val1 + (float)val->val2 / 1000000.0f;
}

static int process_sc7a20h(const struct device *dev)
{
    struct sensor_value accel[3];
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

    
    printk("Accel (m/s^2): X=%.2f, Y=%.2f, Z=%.2f\n",
           sc7a20h_value_to_ms2(&accel[0]),
           sc7a20h_value_to_ms2(&accel[1]),
           sc7a20h_value_to_ms2(&accel[2]));

    return 0;
}

static void trigger_handler(const struct device *dev,
                          const struct sensor_trigger *trigger)
{
    struct sensor_value accel[3];
    int ret;

    ret = sensor_sample_fetch(dev);
    if (ret != 0) {
        printk("sensor_sample_fetch failed: %d\n", ret);
        return;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    if (ret == 0) {
        printk("Accel (m/s^2): X=%.2f, Y=%.2f, Z=%.2f\n",
               sc7a20h_value_to_ms2(&accel[0]),
               sc7a20h_value_to_ms2(&accel[1]),
               sc7a20h_value_to_ms2(&accel[2]));
    }
}

int main(void)
{
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(sc7a20h_csk6_test));
    int ret;

    printk("SC7A20H test start\n");

    if (!device_is_ready(dev)) {
        printk("SC7A20H device not found\n");
        return -1;
    }

#ifdef CONFIG_SC7A20H_TRIGGER
    /* 配置中断触发 */
    struct sensor_trigger trigger = {
        .type = SENSOR_TRIG_DATA_READY,
        .chan = SENSOR_CHAN_ACCEL_XYZ,
    };

    if (sensor_trigger_set(dev, &trigger, trigger_handler) < 0) {
        printk("Cannot configure trigger\n");
        return -1;
    }

    printk("Configured for triggered sampling.\n");
    printk("Move the sensor to generate data!\n");
#else
    printk("Running in polling mode\n");
#endif

    while (1) {
        ret = process_sc7a20h(dev);
        if (ret < 0) {
            printk("Error processing sensor data: %d\n", ret);
        }
        k_sleep(K_MSEC(SLEEP_TIME_MS));
    }

    return 0;
}



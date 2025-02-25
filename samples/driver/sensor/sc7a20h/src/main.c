/*
 * Copyright (c) 2023 Listenai
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

static const char *now_str(void)
{
    static char buf[16]; /* ...HH:MM:SS.MMM */
    uint32_t now = k_uptime_get_32();
    unsigned int ms = now % MSEC_PER_SEC;
    unsigned int s;
    unsigned int min;
    unsigned int h;

    now /= MSEC_PER_SEC;
    s = now % 60U;
    now /= 60U;
    min = now % 60U;
    now /= 60U;
    h = now;

    snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u",
             h, min, s, ms);
    return buf;
}

static int process_sc7a20h(const struct device *dev)
{
    struct sensor_value accel[3];
    int rc = sensor_sample_fetch(dev);

    if (rc == 0) {
        rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    }
    
    if (rc == 0) {
        printf("[%s]: \n  accel %f %f %f m/s/s\n",
               now_str(),
               sensor_value_to_double(&accel[0]),
               sensor_value_to_double(&accel[1]),
               sensor_value_to_double(&accel[2]));
    } else {
        printf("sample fetch/get failed: %d\n", rc);
    }

    return rc;
}

#ifdef CONFIG_SC7A20H_TRIGGER
static struct sensor_trigger trigger;

static void handle_sc7a20h_drdy(const struct device *dev,
                               const struct sensor_trigger *trig)
{
    int rc = process_sc7a20h(dev);
    
    if (rc != 0) {
        printf("cancelling trigger due to failure: %d\n", rc);
        (void)sensor_trigger_set(dev, trig, NULL);
        return;
    }
}
#endif /* CONFIG_SC7A20H_TRIGGER */

int main(void)
{
    const struct device *const sc7a20h = DEVICE_DT_GET(DT_NODELABEL(sc7a20h));

    if (!device_is_ready(sc7a20h)) {
        printf("Device %s is not ready\n", sc7a20h->name);
        return 0;
    }

#ifdef CONFIG_SC7A20H_TRIGGER
    trigger = (struct sensor_trigger) {
        .type = SENSOR_TRIG_DATA_READY,
        .chan = SENSOR_CHAN_ALL,
    };
    if (sensor_trigger_set(sc7a20h, &trigger, handle_sc7a20h_drdy) < 0) {
        printf("Cannot configure trigger\n");
        return 0;
    }
    printk("Configured for triggered sampling.\n");
#endif

    // 修改循环逻辑，使其在任何情况下都能工作
#ifdef CONFIG_SC7A20H_TRIGGER
    // 触发模式下，主线程可以休眠
    while (1) {
        k_sleep(K_SECONDS(10));  // 主线程每10秒醒来一次，可以用于监控或其他任务
    }
#else
    // 非触发模式下，使用轮询方式
    while (1) {
        int rc = process_sc7a20h(sc7a20h);
        if (rc != 0) {
            printf("Error in polling mode: %d\n", rc);
            break;
        }
        k_sleep(K_SECONDS(2));
    }
#endif

    return 0;
}

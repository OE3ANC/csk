/*
 * Copyright (c) 2023 Anhui(Shenzhen) Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

int main(void)
{
	const struct pwm_dt_spec spec = PWM_DT_SPEC_GET(DT_NODELABEL(motor_pwm));

	if (!device_is_ready(spec.dev)) {
		printk("device: %s is not ready\n", spec.dev->name);
	}

	printk("name: %s \n", spec.dev->name);
	printk("channel: %d \n", spec.channel);
	printk("period: %d \n", spec.period);
	printk("flag: %d \n", spec.flags);

	int r = pwm_set_dt(&spec, 1000 * 1000 * 1000, 500 * 1000 * 1000);

	if (r) {
		printk("pwm set dt failed, r:%d\n", r);
	}

	while (1) {
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}

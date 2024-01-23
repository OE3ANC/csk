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

const struct pwm_dt_spec spec = PWM_DT_SPEC_GET(DT_NODELABEL(motor_pwm));

int _pwm_set(uint32_t period_ms,
			     uint32_t pulse_ms)
{
	int err;
	uint64_t pulse_cycles;
	uint64_t period_cycles;
	uint64_t cycles_per_sec;

	err = pwm_get_cycles_per_sec(spec.dev, spec.channel, &cycles_per_sec);
	if (err < 0) {
		return err;
	}

	period_cycles = (period_ms * cycles_per_sec) / MSEC_PER_SEC;
	if (period_cycles > UINT16_MAX) {
		return -ENOTSUP;
	}

	pulse_cycles = (pulse_ms * cycles_per_sec) / MSEC_PER_SEC;
	if (pulse_cycles > UINT16_MAX) {
		return -ENOTSUP;
	}

	return pwm_set_cycles(spec.dev, spec.channel, (uint32_t)period_cycles,
			      (uint32_t)pulse_cycles, spec.flags);
}

int main(void)
{
	int r = 0;

	if (!device_is_ready(spec.dev)) {
		printk("device: %s is not ready\n", spec.dev->name);
		return 0;
	}

	printk("name: %s \n", spec.dev->name);
	printk("channel: %d \n", spec.channel);
	printk("period: %d \n", spec.period);
	printk("flag: %d \n", spec.flags);

	r = _pwm_set(spec.period, spec.period / 2);
	if (r) {
		printk("pwm set failed, r:%d\n", r);
		return 0;
	}

	return 0;
}

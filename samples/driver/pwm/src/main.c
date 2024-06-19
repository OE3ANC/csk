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


#define max_period PWM_MSEC(1U)

const struct pwm_dt_spec spec = PWM_DT_SPEC_GET(DT_NODELABEL(inter_pwm));



int main(void)
{
	if (!device_is_ready(spec.dev)) {
		printk("device: %s is not ready\n", spec.dev->name);
		return 0;
	}

	printk("name: %s \n", spec.dev->name);
	printk("channel: %d \n", spec.channel);
	printk("period: %d \n", spec.period);
	printk("flag: %d \n", spec.flags);
	if(pwm_set_dt(&spec, max_period, max_period / 2U) == 0){
		printk("pwm_set_dt success\n");
	}
	k_sleep(K_SECONDS(4U));
	while(1){}
	return 0;
}

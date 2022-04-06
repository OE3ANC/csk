/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <devicetree.h>
#include <drivers/gpio.h>

// devicetree: LED CAM NODE
#define CAM_LED 		DT_NODELABEL(camleds1)
#define GPIO_LED_CAM 	DT_GPIO_CTLR_BY_IDX(CAM_LED, gpios, 0) 

#define FY_KEY 			DT_NODELABEL(fy_key_0)
#define GPIO_FY_KEY 	DT_GPIO_CTLR_BY_IDX(FY_KEY, gpios, 0) 

void cam_led_test(){
	const struct device *cam_led;
	int ret;
	bool led_is_on = true;
	cam_led = DEVICE_DT_GET(GPIO_LED_CAM);
	if (!device_is_ready(cam_led)) {
		printk("Error: PWM device %s is not ready\n", cam_led->name);
		return;
	}
	ret = gpio_pin_configure(cam_led, 7, GPIO_OUTPUT_ACTIVE | 0);
	if (ret < 0) {
		return;
	}

	for(int _=0; _<10; _++){
		gpio_pin_set(cam_led, 7, (int)led_is_on);
		led_is_on = !led_is_on;
		k_msleep(500);
	}
	gpio_pin_set(cam_led, 7, 1);			//turn off the cam led
}


// void fy_key_test(){
// 	const struct device *fy_key,*cam_led;	
// 	cam_led = DEVICE_DT_GET(GPIO_LED_CAM);
// 	fy_key = DEVICE_DT_GET(GPIO_FY_KEY);

// 	gpio_pin_configure(cam_led, 7, GPIO_OUTPUT_ACTIVE | 1);
// 	gpio_pin_configure(fy_key,  5, GPIO_INPUT);


// }


void main(void)
{	
	cam_led_test();
	// fy_key_test();
}

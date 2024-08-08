/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define GPIOA_7_NODE DT_ALIAS(gpioa7)

static const struct gpio_dt_spec gpioa7 = GPIO_DT_SPEC_GET(GPIOA_7_NODE, gpios);


static int gpio_a7_enable(void)
{

    gpio_pin_set_dt(&gpioa7, 0);

    return 0;
}

static int gpio_a7_disenable(void)
{

    gpio_pin_set_dt(&gpioa7, 1);

    return 0;
}
int main(void)
{
	printk("GPIO sample application\n");
	if (gpioa7.port == NULL) {
        printk("Failed to get GPIOA7 device\n");
        return -ENODEV;
    }
	gpio_pin_configure_dt(&gpioa7, GPIO_OUTPUT_ACTIVE);


    while (1) {

		gpio_a7_enable();
        k_msleep(1000);
		gpio_a7_disenable();
		k_msleep(1000);
    }
	return 0;
}

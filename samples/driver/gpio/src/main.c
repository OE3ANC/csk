/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define GPIOA_1_NODE DT_ALIAS(gpioa1)

static const struct gpio_dt_spec gpioa1 = GPIO_DT_SPEC_GET(GPIOA_1_NODE, gpios);


static int gpio_a1_enable(void)
{

    gpio_pin_set_dt(&gpioa1, 0);

    return 0;
}

static int gpio_a1_disenable(void)
{

    gpio_pin_set_dt(&gpioa1, 1);

    return 0;
}
int main(void)
{
	printk("GPIO sample application\n");
	if (gpioa1.port == NULL) {
        printk("Failed to get GPIOA1 device\n");
        return -ENODEV;
    }
	gpio_pin_configure_dt(&gpioa1, GPIO_OUTPUT_ACTIVE);


    while (1) {

		gpio_a1_enable();
        k_msleep(1000);
		gpio_a1_disenable();
		k_msleep(1000);
    }
	return 0;
}

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
#include <zephyr/logging/log.h>


#define SW0_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec key_3 = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});

static struct gpio_callback key_3_cb_data;
K_SEM_DEFINE(key_sem, 0, 1);


void key_3_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{	
	k_sem_give(&key_sem);
}

int key_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&key_3)) {
		printk("Error: key_3 device %s is not ready\n", key_3.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&key_3, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, key_3.port->name,
		       key_3.pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&key_3, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
		       key_3.port->name, key_3.pin);
		return ret;
	}

	gpio_init_callback(&key_3_cb_data, key_3_pressed, BIT(key_3.pin));
	gpio_add_callback(key_3.port, &key_3_cb_data);
	printk("Set up key_3 at %s pin %d\n", key_3.port->name, key_3.pin);

	return 0;
}
void key_interrupt(void){
	int err;
	while(1){
		err = k_sem_take(&key_sem, K_NO_WAIT);
		if(err == 0){
			printk("LIGHT ON LED\n");

		}
	}
}
int main(void)
{
	int r = 0;
	r = key_init();
	if(r == 0){
		printk("key_init success\n");
	}
	key_interrupt();
	return 0;
}

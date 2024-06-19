/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>

#define DELAY_TIME_SEC 6
#define NEW_BAUDRATE   115200

void print_uart_config(const struct uart_config *cfg)
{
	printk("baudrate: %d\n", cfg->baudrate);
	printk("data_bits: %d\n", cfg->data_bits);
	printk("stop_bits: %d\n", cfg->stop_bits);
	printk("flow_ctrl: %d\n", cfg->flow_ctrl);
	printk("parity: %d\n", cfg->parity);
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	struct uart_config cfg = {0};

	if (!device_is_ready(dev)) {
		printk("device:%s is not ready!\n", dev->name);
		return -1;
	}

	if (uart_config_get(dev, &cfg) != 0) {
		printk("uart config get failed.\n");
		return -1;
	}

	if (cfg.baudrate == NEW_BAUDRATE) {
		printk("current baudrate is same as new baudrate(%d).\n", NEW_BAUDRATE);
		return -1;
	}

	printk("current uart configuration:\n");
	print_uart_config(&cfg);

	int c = DELAY_TIME_SEC;
	while (c) {
		printk("baudrate will change to %d after %d sec.\n", NEW_BAUDRATE, c--);
		k_msleep(1000);
	}

	cfg.baudrate = NEW_BAUDRATE;

	if (uart_configure(dev, &cfg) != 0) {
		printk("uart configure failed.\n");
		return -1;
	}

	printk("uart configuration succeeded.\n");

	if (uart_config_get(dev, &cfg) != 0) {
		printk("uart config get failed.\n");
		return -1;
	}
	printk("config result:\n");
	print_uart_config(&cfg);

	while (1) {
		printk("hello world\n");
		k_msleep(500);
	}

	return 0;
}

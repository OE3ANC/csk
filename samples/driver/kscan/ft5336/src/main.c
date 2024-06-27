/*
 * Copyright (c) 2023 Anhui(Shenzhen) Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/kscan.h>
#include <soc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

static void kb_callback(const struct device *dev, uint32_t row, uint32_t col, bool pressed)
{
	ARG_UNUSED(dev);

	printk("Key row = %d col = %d Pressed = %u\n", row, col, pressed);
}

int main(void)
{
	const struct device *kscan_dev = DEVICE_DT_GET(DT_NODELABEL(ft5336));
	int ret = 0;

	if (!device_is_ready(kscan_dev)) {
		printk("kscan device  not ready \n");
		return -1;
	}

	ret = kscan_config(kscan_dev, kb_callback);
	if (ret != 0) {
		printk("kscan config error \n");
		return -1;
	}
	kscan_enable_callback(kscan_dev);

	printk("ft5336 sample application start\n");

	return 0;
}

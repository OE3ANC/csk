/*
 * Copyright (c) 2021 listenai Intelligent Technology (anhui) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT listenai_csk_pinmux

#include "IOMuxManager.h"
#include "AON_IOMuxManager.h"

#include <soc.h>
#include <zephyr/drivers/pinctrl.h>

#define DEV_CFG(dev) ((const struct pinmux_csk_config *const)(dev)->config)

struct pinmux_csk_config {
	/* pinmux port control registe */
	uintptr_t reg_addr;
};

static int pinmux_set(const struct device *dev, uint32_t pin_num, uint32_t func)
{
	ARG_UNUSED(dev);
	uint8_t pad;
	const struct pinmux_csk_config *config = DEV_CFG(dev);
	if (IOMuxManager_GetPad(config->reg_addr, &pad)) {
		return -EINVAL;
	}
	if (IOMuxManager_PinConfigure(pad, pin_num, func)) {
		return -EINVAL;
	}

	// Alway on domain: CSK_AON_IOMUX_PAD_B, PB0~PB5
	if (func & 0x80) {
		// printk("[%d:%s] pin_num: %d, func: 0x%X\n", __LINE__, __FUNCTION__, pin_num, func);
		if (AON_IOMuxManager_PinConfigure(pad, pin_num, func & 0x7F)) {
			return -EINVAL;
		}
	}

	return 0;
}

static int csk6_pin_configure(const struct device *pinctrls, uint8_t pin, uint8_t alt_fun)
{
	if(pinctrls == NULL){
		return 0;
	}
	
	pinmux_set(pinctrls, pin, alt_fun);		

	return 0;		
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	int ret = 0;

	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		ret = csk6_pin_configure(pins[i].pinctrls, pins[i].pin, pins[i].alt_func);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int csk_pinmux_initialize(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/* Initialize using PRE_KERNEL_1 priority so that GPIO can use the pin_num
 * mux driver.
 */
#define PINMUX_CSK_INIT(index)                                                                     \
	static const struct pinmux_csk_config pinmux_csk_config##index = {                         \
		.reg_addr = DT_INST_REG_ADDR(index),                                               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, &csk_pinmux_initialize, NULL, NULL,                           \
			      &pinmux_csk_config##index, PRE_KERNEL_1,                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(PINMUX_CSK_INIT)

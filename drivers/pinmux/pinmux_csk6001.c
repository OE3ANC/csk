/*
 * Copyright (c) 2021 listenai Systems (anhui) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT listenai_csk_pinmux


#include <drivers/pinmux.h>
#include <hal/IOMuxManager.h>


static int pinmux_set(const struct device *dev, uint32_t pin, uint32_t func)
{
	ARG_UNUSED(dev);

	if ((func > PINMUX_FUNC_T) || (pin > CSK_PINMIX_NUM_MAX)) {
		return -EINVAL;
	}

	if(csk_pinmux_set_func(pin, func)){
		return -EINVAL;
	}

	return 0;
}

static int pinmux_get(const struct device *dev, uint32_t pin, uint32_t *func)
{
	if (pin > CSK_PINMIX_NUM_MAX) {
		return -EINVAL;
	}

	if(csk_pinmux_get_func(pin, func)){
		return -EINVAL;
	}

	return 0;
}


static int pinmux_pullup(const struct device *dev, uint32_t pin, uint8_t func)
{
	ARG_UNUSED(dev);

	if ((func > PINMUX_FUNC_T) || (pin > CSK_PINMIX_NUM_MAX)) {
		return -EINVAL;
	}

	switch (func) {
	case PINMUX_PULLUP_DISABLE:
		return csk_pinmux_set_mode(pin, CSK_PINMUX_PULLUP_DISABLE);
		break;
	case PINMUX_PULLUP_ENABLE:
		return csk_pinmux_set_mode(pin, CSK_PINMUX_PULLUP_ENABLE);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int pinmux_input(const struct device *dev, uint32_t pin, uint8_t func)
{
	/* no support */
	return -ENOTSUP;
}

static struct pinmux_driver_api csk_pinmux_drv_api = {
	.set = pinmux_set,
	.get = pinmux_get,
	.pullup = pinmux_pullup,
	.input = pinmux_input
};

static int csk_pinmux_initialize(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/* Initialize using PRE_KERNEL_1 priority so that GPIO can use the pin
 * mux driver.
 */
DEVICE_DT_INST_DEFINE(0, &csk_pinmux_initialize,
		    NULL, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &csk_pinmux_drv_api);

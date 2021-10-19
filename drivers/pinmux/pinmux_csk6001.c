/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 listenai Systems (anhui) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT listenai_csk_pinmux

#include <kernel.h>
#include <arch/cpu.h>
#include <sys/__assert.h>
#include <soc.h>
#include <init.h>

#include <hal/IOMuxManager.h>

#include <errno.h>
#include <drivers/pinmux.h>

#define  CSK_PIN_A_NUM_MAX      31
#define  CSK_PIN_B_NUM_MAX      15

#define  CSK_PAD(x)  ((x)<CSK_PIN_A_NUM_MAX ? CSK_IOMUX_PAD_A:CSK_IOMUX_PAD_B)
#define  CSK_PIN_NUM(x)  ((x)<CSK_PIN_A_NUM_MAX ? x:(x-CSK_PIN_A_NUM_MAX))



static int pinmux_set(const struct device *dev, uint32_t pin, uint32_t func)
{
	ARG_UNUSED(dev);

	if ((func > PINMUX_FUNC_T) || (pin > (CSK_PIN_A_NUM_MAX+CSK_PIN_B_NUM_MAX))) {
		return -EINVAL;
	}
	uint8_t pad = CSK_PAD(pin);
	uint8_t pin_num = CSK_PIN_NUM(pin);

	return IOMuxManager_PinConfigure(pad, pin_num, func);
}

static int pinmux_get(const struct device *dev, uint32_t pin, uint32_t *func)
{
	ARG_UNUSED(dev);

	if (pin > (CSK_PIN_A_NUM_MAX+CSK_PIN_B_NUM_MAX)) {
		return -EINVAL;
	}

	/* TODO*/

	return 0;
}


static int pinmux_pullup(const struct device *dev, uint32_t pin, uint8_t func)
{
	ARG_UNUSED(dev);

	if (pin > (CSK_PIN_A_NUM_MAX+CSK_PIN_B_NUM_MAX)) {
		return -EINVAL;
	}

	uint8_t pad = CSK_PAD(pin);
	uint8_t pin_num = CSK_PIN_NUM(pin);

	switch (func) {
	case PINMUX_PULLUP_DISABLE:
		IOMuxManager_ModeConfigure(pad, pin_num, HAL_IOMUX_PULLDOWN_MODE);
		break;
	case PINMUX_PULLUP_ENABLE:
		IOMuxManager_ModeConfigure(pad, pin_num, HAL_IOMUX_PULLUP_MODE);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int pinmux_input(const struct device *dev, uint32_t pin, uint8_t func)
{
	ARG_UNUSED(dev);

   	/* TODO*/
	return 0;
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

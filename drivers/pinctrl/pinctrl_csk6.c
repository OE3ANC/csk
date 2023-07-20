/*
 * Copyright (c) 2021 listenai Intelligent Technology (anhui) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "IOMuxManager.h"
#include "AON_IOMuxManager.h"

#include <soc.h>
#include <zephyr/drivers/pinctrl.h>

static int pinmux_set(uint32_t pin_num, uint32_t func)
{
	uint8_t pad = CSK_IOMUX_PAD_A;

	if (IOMuxManager_PinConfigure(pad, pin_num, func)) {
		return -EINVAL;
	}
#if !(CONFIG_BOARD_LS_ADSP_CSK6)
	// Alway on domain: CSK_AON_IOMUX_PAD_B, PB0~PB5
	if (func & 0x80) {
		// printk("[%d:%s] pin_num: %d, func: 0x%X\n", __LINE__, __FUNCTION__, pin_num, func);
		if (AON_IOMuxManager_PinConfigure(pad, pin_num, func & 0x7F)) {
			return -EINVAL;
		}
	}
#endif
	return 0;
}

static int csk6_pin_configure(uint8_t pin, uint8_t alt_fun)
{
	
	pinmux_set(pin, alt_fun);		

	return 0;		
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	int ret = 0;

	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		ret = csk6_pin_configure(pins[i].pin, pins[i].alt_func);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

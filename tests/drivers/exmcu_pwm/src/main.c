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
#include <zephyr/ztest.h>

ZTEST(exmcu_pwd_test, test_exmcu_pwm_api)
{
	int err;
	const struct pwm_dt_spec spec = PWM_DT_SPEC_GET(DT_NODELABEL(motor_pwm));

	zassert_true(device_is_ready(spec.dev), "device: %s is not ready", spec.dev->name);

	err = pwm_set_dt(&spec, 1000 * 1000 * 1000, 500 * 1000 * 1000);
	zassert_ok(err, "pwm set dt failed, r:%d", err);
}

ZTEST_SUITE(exmcu_pwd_test, NULL, NULL, NULL, NULL, NULL);

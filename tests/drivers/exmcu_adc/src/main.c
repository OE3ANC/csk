/*
 * Copyright (c) 2023 Anhui(Shenzhen) Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define ADC_NUM_CHANNELS DT_PROP_LEN(DT_PATH(zephyr_user), io_channels)
#define ADC_NODE DT_PHANDLE(DT_PATH(zephyr_user), io_channels)

/* Common settings supported by most ADCs */
#define ADC_RESOLUTION 10
#define ADC_GAIN       ADC_GAIN_1
#define ADC_REFERENCE  ADC_REF_INTERNAL

static uint8_t channel_ids = DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 0);

static int16_t sample_buffer = 0;

struct adc_channel_cfg channel_cfg = {
	.gain = ADC_GAIN_1,
	.reference = ADC_REFERENCE,
	.channel_id = 0,
};

struct adc_sequence sequence = {
	/* individual channels will be added below */
	.channels = 0,
	.buffer = (void *)&sample_buffer,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(sample_buffer),
	.resolution = ADC_RESOLUTION,
};

static const struct device *dev_adc = DEVICE_DT_GET(ADC_NODE);
static const struct device *dev_gpio = DEVICE_DT_GET(DT_NODELABEL(gpioa));

#define TEST_GPIO_PIN 0

static void *exmcu_adc_test_setup(void)
{

	zassert_true(device_is_ready(dev_adc), "ADC device not found");
	zassert_true(device_is_ready(dev_gpio), "dev_gpio device not found");

	gpio_pin_configure(dev_gpio, TEST_GPIO_PIN, GPIO_OUTPUT);

	channel_cfg.channel_id = channel_ids;
	adc_channel_setup(dev_adc, &channel_cfg);
	sequence.channels |= BIT(channel_ids);

	return NULL;
}

ZTEST(exmcu_adc, test_adc_read_high_vol)
{
	int err;
	int32_t mv_value;
	int32_t adc_vref;

	gpio_pin_set_raw(dev_gpio, TEST_GPIO_PIN, 1);
	k_msleep(1000);

	adc_vref = adc_ref_internal(dev_adc);
	err = adc_read(dev_adc, &sequence);
	zassert_ok(err, "adc read failed");

	mv_value = sample_buffer;
	adc_raw_to_millivolts(adc_vref, ADC_GAIN_1, ADC_RESOLUTION, &mv_value);
	zassert_between_inclusive(mv_value, adc_vref - 200, adc_vref + 200,
				  "adc read high vol failed, ref:%d, raw: %d, mv:%d", adc_vref,
				  sample_buffer, mv_value);
}

ZTEST(exmcu_adc, test_adc_read_low_vol)
{
	int err;
	int32_t mv_value;
	int32_t adc_vref;

	gpio_pin_set_raw(dev_gpio, TEST_GPIO_PIN, 0);
	k_msleep(1000);

	adc_vref = adc_ref_internal(dev_adc);
	err = adc_read(dev_adc, &sequence);
	zassert_ok(err, "adc read failed");

	mv_value = sample_buffer;
	adc_raw_to_millivolts(adc_vref, ADC_GAIN_1, ADC_RESOLUTION, &mv_value);
	zassert_between_inclusive(mv_value, 0, 200, "adc read low vol failed, ref:%d, raw: %d, mv:%d",
				  adc_vref, sample_buffer, mv_value);
}

ZTEST_SUITE(exmcu_adc, NULL, exmcu_adc_test_setup, NULL, NULL, NULL);

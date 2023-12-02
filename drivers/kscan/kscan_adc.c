/*
 * Copyright (c) 2023 Anhui Listenai Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adc_keys

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(kscan_adc, CONFIG_KSCAN_LOG_LEVEL);

#define DIFF(a, b) ((a) > (b) ? (a) - (b) : (b) - (a))

struct kscan_adc_press {
	uint32_t press_mv;
	uint32_t key_bits;
};

struct kscan_adc_config {
	struct adc_dt_spec channel;
	uint32_t keyup_mv;
	const struct kscan_adc_press *presses;
	uint32_t press_count;
};

struct kscan_adc_data {
	kscan_callback_t cb;

	const struct device *self;
	struct k_work work;
	struct k_timer timer;

	struct adc_sequence seq;

	uint32_t last_key_bits;
};

static void kscan_adc_process(const struct device *dev);

static void kscan_adc_work_handler(struct k_work *work)
{
	struct kscan_adc_data *data = CONTAINER_OF(work, struct kscan_adc_data, work);
	const struct device *dev = data->self;

	kscan_adc_process(dev);
}

static void kscan_adc_timer_handler(struct k_timer *timer)
{
	struct kscan_adc_data *data = CONTAINER_OF(timer, struct kscan_adc_data, timer);

	k_work_submit(&data->work);
}

static int kscan_adc_configure(const struct device *dev, kscan_callback_t callback)
{
	struct kscan_adc_data *data = dev->data;

	data->cb = callback;

	return 0;
}

static int kscan_adc_disable_callback(const struct device *dev)
{
	struct kscan_adc_data *data = dev->data;

	k_timer_stop(&data->timer);

	return 0;
}

static int kscan_adc_enable_callback(const struct device *dev)
{
	struct kscan_adc_data *data = dev->data;

	k_timer_start(&data->timer, K_MSEC(CONFIG_KSCAN_ADC_PERIOD_MS),
		      K_MSEC(CONFIG_KSCAN_ADC_PERIOD_MS));

	return 0;
}

static inline uint32_t kscan_adc_read(const struct device *dev)
{
	const struct kscan_adc_config *cfg = dev->config;
	struct kscan_adc_data *data = dev->data;
	uint16_t sample_raw;
	int32_t sample_mv;
	int ret;

	data->seq.buffer = &sample_raw;
	data->seq.buffer_size = sizeof(sample_raw);

	ret = adc_read(cfg->channel.dev, &data->seq);
	if (ret) {
		LOG_ERR("ADC read failed %d", ret);
		return cfg->keyup_mv;
	}

	sample_mv = (int32_t)sample_raw;
	adc_raw_to_millivolts_dt(&cfg->channel, &sample_mv);

	return (uint32_t)sample_mv;
}

static void kscan_adc_process(const struct device *dev)
{
	const struct kscan_adc_config *cfg = dev->config;
	struct kscan_adc_data *data = dev->data;
	uint32_t sample_mv;
	uint32_t diff, closest = UINT32_MAX;
	uint32_t key_bits = 0;

	if (!data->cb) {
		return;
	}

	sample_mv = kscan_adc_read(dev);

	for (uint32_t i = 0; i < cfg->press_count; i++) {
		diff = DIFF(sample_mv, cfg->presses[i].press_mv);
		if (diff < closest) {
			closest = diff;
			key_bits = cfg->presses[i].key_bits;
		}
	}

	diff = DIFF(sample_mv, cfg->keyup_mv);
	if (diff < closest) {
		key_bits = 0;
	}

	if (key_bits == data->last_key_bits) {
		return;
	}

	for (uint32_t i = 0; i < 32; i++) {
		if ((data->last_key_bits & BIT(i)) && !(key_bits & BIT(i))) {
			data->cb(dev, i, 0, false);
		}

		if (!(data->last_key_bits & BIT(i)) && (key_bits & BIT(i))) {
			data->cb(dev, i, 0, true);
		}
	}

	data->last_key_bits = key_bits;
}

static int kscan_adc_init(const struct device *dev)
{
	const struct kscan_adc_config *cfg = dev->config;
	struct kscan_adc_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->channel.dev)) {
		LOG_ERR("ADC controller device %s not ready", cfg->channel.dev->name);
		return -ENODEV;
	}

	ret = adc_channel_setup_dt(&cfg->channel);
	if (ret) {
		LOG_ERR("ADC channel setup failed %d", ret);
		return ret;
	}

	adc_sequence_init_dt(&cfg->channel, &data->seq);

	k_work_init(&data->work, kscan_adc_work_handler);
	k_timer_init(&data->timer, kscan_adc_timer_handler, NULL);

#if defined(CONFIG_KSCAN_LOG_LEVEL_DBG)
	for (uint32_t i = 0; i < cfg->press_count; i++) {
		LOG_DBG("* Level %d: %d mV, keys 0x%08x", i, cfg->presses[i].press_mv,
			cfg->presses[i].key_bits);
	}
#endif /* CONFIG_KSCAN_LOG_LEVEL_DBG */

	return 0;
}

static const struct kscan_driver_api kscan_adc_api = {
	.config = kscan_adc_configure,
	.disable_callback = kscan_adc_disable_callback,
	.enable_callback = kscan_adc_enable_callback,
};

#define KSCAN_ADC_INST(n)                                                                          \
	static struct kscan_adc_data kscan_adc_data_##n = {                                        \
		.self = DEVICE_DT_INST_GET(n),                                                     \
	};                                                                                         \
                                                                                                   \
	static const uint32_t kscan_adc_presses_##n[] = DT_INST_PROP(n, press_thresholds_mv);      \
                                                                                                   \
	static const struct kscan_adc_config kscan_adc_cfg_##n = {                                 \
		.channel = ADC_DT_SPEC_INST_GET(n),                                                \
		.keyup_mv = DT_INST_PROP(n, keyup_threshold_mv),                                   \
		.presses = (const struct kscan_adc_press *)kscan_adc_presses_##n,                  \
		.press_count = DT_INST_PROP_LEN(n, press_thresholds_mv) / 2,                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, kscan_adc_init, NULL, &kscan_adc_data_##n, &kscan_adc_cfg_##n,    \
			      POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY, &kscan_adc_api);

DT_INST_FOREACH_STATUS_OKAY(KSCAN_ADC_INST)

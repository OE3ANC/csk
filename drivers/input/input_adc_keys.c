/*
 * Copyright (c) 2023 Anhui Listenai Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_adc_keys

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(zephyr_adc_keys, CONFIG_INPUT_LOG_LEVEL);

struct adc_keys_key_config {
	uint32_t press_mv;
	uint32_t zephyr_code;
};

struct adc_keys_config {
	struct adc_dt_spec channel;
	uint32_t sample_period_ms;
	uint32_t keyup_mv;
	const struct adc_keys_key_config *key_cfg;
	uint32_t key_cnt;
};

struct adc_keys_key_data {
	uint8_t key_state;
};

struct adc_keys_data {
	const struct device *self;
	struct k_work work;
	struct k_timer timer;
	struct adc_sequence seq;
	struct adc_keys_key_data *key_data;
};

static void adc_keys_process(const struct device *dev);

static void adc_keys_work_handler(struct k_work *work)
{
	struct adc_keys_data *data = CONTAINER_OF(work, struct adc_keys_data, work);
	const struct device *dev = data->self;

	adc_keys_process(dev);
}

static void adc_keys_timer_handler(struct k_timer *timer)
{
	struct adc_keys_data *data = CONTAINER_OF(timer, struct adc_keys_data, timer);

	k_work_submit(&data->work);
}

static inline uint32_t adc_keys_read(const struct device *dev)
{
	const struct adc_keys_config *cfg = dev->config;
	struct adc_keys_data *data = dev->data;
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

static void adc_keys_process(const struct device *dev)
{
	const struct adc_keys_config *cfg = dev->config;
	struct adc_keys_data *data = dev->data;
	uint32_t sample_mv, closest_mv;
	uint32_t diff, closest_diff = UINT32_MAX;
	const struct adc_keys_key_config *key_cfg;
	struct adc_keys_key_data *key_data;
	uint8_t key_state;

	sample_mv = adc_keys_read(dev);

	/*
	 * Find the closest key press threshold to the sample value.
	 */

	for (uint32_t i = 0; i < cfg->key_cnt; i++) {
		diff = abs(sample_mv - cfg->key_cfg[i].press_mv);
		if (diff < closest_diff) {
			closest_diff = diff;
			closest_mv = cfg->key_cfg[i].press_mv;
		}
	}

	diff = abs(sample_mv - cfg->keyup_mv);
	if (diff < closest_diff) {
		closest_mv = cfg->keyup_mv;
	}

	LOG_DBG("sample=%d mV, closest=%d mV, diff=%d mV", sample_mv, closest_mv, closest_diff);

	/*
	 * Report the key event if the key state has changed. Note that multiple
	 * keys may have the same press threshold, which is the mixed voltage
	 * that these keys are simultaneously pressed.
	 */

	for (uint32_t i = 0; i < cfg->key_cnt; i++) {
		key_cfg = &cfg->key_cfg[i];
		key_data = &data->key_data[i];

		key_state = (closest_mv == key_cfg->press_mv);
		if (key_state != key_data->key_state) {
			key_data->key_state = key_state;
			LOG_DBG("Report event %s %d, code=%d", dev->name, key_state,
				key_cfg->zephyr_code);
			input_report_key(dev, key_cfg->zephyr_code, key_state, true, K_FOREVER);
		}
	}
}

static int adc_keys_init(const struct device *dev)
{
	const struct adc_keys_config *cfg = dev->config;
	struct adc_keys_data *data = dev->data;
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

	k_work_init(&data->work, adc_keys_work_handler);
	k_timer_init(&data->timer, adc_keys_timer_handler, NULL);

#if defined(CONFIG_INPUT_LOG_LEVEL_DBG)
	for (uint32_t i = 0; i < cfg->key_cnt; i++) {
		LOG_DBG("* key %d: threshold=%d mV code=%d", i, cfg->key_cfg[i].press_mv,
			cfg->key_cfg[i].zephyr_code);
	}
#endif /* CONFIG_INPUT_LOG_LEVEL_DBG */

	k_timer_start(&data->timer, K_MSEC(cfg->sample_period_ms), K_MSEC(cfg->sample_period_ms));

	return 0;
}

#define ADC_KEYS_KEY_CFG_CODE(node_id, prop, idx)                                                  \
	{                                                                                          \
		.press_mv = DT_PROP_BY_IDX(node_id, prop, idx),                                    \
		.zephyr_code = DT_PROP(node_id, zephyr_code),                                      \
	}

#define ADC_KEYS_KEY_CFG(node_id)                                                                  \
	DT_FOREACH_PROP_ELEM_SEP(node_id, press_thresholds_mv, ADC_KEYS_KEY_CFG_CODE, (, ))

#define ADC_KEYS_INST(n)                                                                           \
	static const struct adc_keys_key_config adc_keys_key_cfg_##n[] = {                         \
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(n, ADC_KEYS_KEY_CFG, (, ))};                 \
                                                                                                   \
	static const struct adc_keys_config adc_keys_cfg_##n = {                                   \
		.channel = ADC_DT_SPEC_INST_GET(n),                                                \
		.sample_period_ms = DT_INST_PROP(n, sample_period_ms),                             \
		.keyup_mv = DT_INST_PROP(n, keyup_threshold_mv),                                   \
		.key_cfg = adc_keys_key_cfg_##n,                                                   \
		.key_cnt = ARRAY_SIZE(adc_keys_key_cfg_##n),                                       \
	};                                                                                         \
                                                                                                   \
	static struct adc_keys_key_data adc_keys_key_data_##n[ARRAY_SIZE(adc_keys_key_cfg_##n)];   \
                                                                                                   \
	static struct adc_keys_data adc_keys_data_##n = {                                          \
		.self = DEVICE_DT_INST_GET(n),                                                     \
		.key_data = adc_keys_key_data_##n,                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, adc_keys_init, NULL, &adc_keys_data_##n, &adc_keys_cfg_##n,       \
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(ADC_KEYS_INST)

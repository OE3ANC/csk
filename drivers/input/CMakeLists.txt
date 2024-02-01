# SPDX-License-Identifier: Apache-2.0

zephyr_library_amend()

zephyr_library_sources_ifdef(CONFIG_INPUT_ADC_KEYS input_adc_keys.c)

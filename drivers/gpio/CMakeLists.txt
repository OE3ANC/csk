# SPDX-License-Identifier: Apache-2.0

zephyr_library_amend()

zephyr_library_sources_ifdef(CONFIG_GPIO_CSK6 gpio_csk6.c)
zephyr_library_sources_ifdef(CONFIG_GPIO_CSK6_CH32V003 gpio_csk6_ch32v003.c)

zephyr_library_include_directories_ifdef(CONFIG_GPIO_CSK6 ${ZEPHYR_BASE}/drivers/gpio)
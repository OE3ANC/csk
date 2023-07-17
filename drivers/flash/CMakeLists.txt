# SPDX-License-Identifier: Apache-2.0

zephyr_library_amend()

zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_CSK6 flash_csk6.c)

# SPDX-License-Identifier: Apache-2.0

zephyr_library_amend()

zephyr_library_sources_ifdef(CONFIG_WDT_CSK6  wdt_csk6.c)

zephyr_library_sources_ifdef(CONFIG_AP_WDT_CSK  ap_wdt_csk6.c)
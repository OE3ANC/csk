# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories_ifdef(CONFIG_WIFI_CSK6001
  include
  )

zephyr_library_sources_ifdef(CONFIG_WIFI_CSK6001
  src/csk_wifi.c
  src/csk_wifi_api.c
  )

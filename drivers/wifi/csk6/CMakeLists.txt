# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories_ifdef(CONFIG_WIFI_CSK6
  include
  )

zephyr_library_sources_ifdef(CONFIG_ESP_HOSTED
  src/csk_wifi.c
  src/csk_wifi_api.c
  )

zephyr_library_sources_ifdef(CONFIG_CSK_WIFI_ESP_UART
  src/esp_offload_sock/esp_offload_socket.c
  src/esp_offload_sock/csk_wifi_offload.c
  )

zephyr_library_sources_ifdef(CONFIG_XR819S
  src/csk_wifi_819s.c
  src/csk_wifi_api_819s.c
  )


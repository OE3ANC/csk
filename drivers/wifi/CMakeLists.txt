# SPDX-License-Identifier: Apache-2.0

zephyr_library_amend()
add_subdirectory_ifdef(CONFIG_WIFI_CSK6      csk6)

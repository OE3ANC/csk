# SPDX-License-Identifier: Apache-2.0

zephyr_library_amend()
add_subdirectory_ifdef(CONFIG_BT     hci)
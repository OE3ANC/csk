# SPDX-License-Identifier: Apache-2.0

zephyr_library_amend()

zephyr_library_sources_ifdef(CONFIG_KSCAN_CSK6  kscan_csk6.c)
zephyr_library_sources_ifdef(CONFIG_KSCAN_BL6XXX  kscan_bl6xxx.c)
zephyr_library_sources_ifdef(CONFIG_KSCAN_FT5336  kscan_ft5336.c)

# SPDX-License-Identifier: Apache-2.0

zephyr_library_amend()

zephyr_library_sources_ifdef(CONFIG_HWINFO_CSK6 hwinfo_csk6.c)

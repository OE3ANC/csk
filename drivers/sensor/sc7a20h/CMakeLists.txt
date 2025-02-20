# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(sc7a20h.c)
zephyr_library_sources_ifdef(CONFIG_SC7A20H_TRIGGER sc7a20h_trigger.c)

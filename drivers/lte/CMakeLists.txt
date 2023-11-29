# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(lte_driver)

zephyr_library_sources_ifdef(CONFIG_LTE_EC718  ec718.c)
zephyr_library_sources_ifdef(CONFIG_LTE_EC718  ec718_conn.c)
zephyr_library_sources_ifdef(CONFIG_LTE_EC718  protocol.c)
zephyr_library_sources_ifdef(CONFIG_LTE_EC718  protocol_socket_host.c)
zephyr_include_directories(.)

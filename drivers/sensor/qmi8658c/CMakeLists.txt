# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(qmi8658c.c)
zephyr_library_sources_ifdef(CONFIG_QMI8658C_TRIGGER qmi8658c_trigger.c)

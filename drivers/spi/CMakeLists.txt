# SPDX-License-Identifier: Apache-2.0

zephyr_library_amend()

zephyr_library_sources_ifdef(CONFIG_SPI_CSK6  spi_csk6.c)
zephyr_library_include_directories_ifdef(CONFIG_SPI_CSK6 ${ZEPHYR_BASE}/drivers/spi)

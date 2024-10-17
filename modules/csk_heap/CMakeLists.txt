# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_include_directories(
  .
  ${ZEPHYR_BASE}/lib/os
)

zephyr_library_sources(
  ./csk_malloc.c
)
  
# Copyright (c) 2024 ListenAI
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_cc_option(-O0)
SET(HAL_CSK ${ZEPHYR_HAL_CSK_MODULE_DIR})
zephyr_compile_options(-DARMCM30_DSP_FP -DPLATFORM_AP -DCSK6001 -DIC_BOARD)
zephyr_compile_options(-DHAL_CUSTOM_CONFIG_FILE="hal_config.h")

zephyr_include_directories(./)
zephyr_include_directories(${HAL_CSK})
zephyr_include_directories(${HAL_CSK}/include)
zephyr_include_directories(${HAL_CSK}/include/reg)
zephyr_include_directories(${HAL_CSK}/include/sdmmc)

file(GLOB_RECURSE HAL_CSK  ${HAL_CSK}/src/*.c)
zephyr_library_sources(${HAL_CSK})

# SPDX-License-Identifier: Apache-2.0
zephyr_include_directories(.)
zephyr_sources(
  firmware_verification_table.S
  soc.c
  )

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "linker script for csk")

# make sure csk6_boot_tag.ld at the end of vectors
zephyr_linker_sources(ROM_START SORT_KEY 0x0vectors0boottag csk6_boot_tag.ld)
zephyr_sources_ifdef(CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_MPU_REGIONS mpu_regions.c)
zephyr_sources_ifdef(CONFIG_CSK6_FATAL fatal.c)
zephyr_linker_sources(SECTIONS csk6_psram.ld)

if(NOT CONFIG_BOOTLOADER_MCUBOOT)
  if(CONFIG_BUILD_OUTPUT_HEX)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
      ${PYTHON_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/patch_hex.py
      ${KERNEL_HEX_NAME}
    )
  endif()

  if(CONFIG_BUILD_OUTPUT_BIN)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
      ${PYTHON_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/patch_bin.py
      ${KERNEL_BIN_NAME}
    )
  endif()

  if(CONFIG_BUILD_OUTPUT_BIN AND CONFIG_BUILD_OUTPUT_UF2)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
      ${PYTHON_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/patch_uf2.py
      ${KERNEL_UF2_NAME}
      ${KERNEL_BIN_NAME}
    )
  endif()
endif()

# SPDX-License-Identifier: Apache-2.0

zephyr_ld_options(-mthumb -mapcs-frame -mthumb-interwork -static -Wl,-gc-sections --specs=nosys.specs)

zephyr_compile_options(-Wall -Wno-unused -Wno-comment -MMD -ffunction-sections -fdata-sections -mthumb -gdwarf-2 -mapcs-frame -mthumb-interwork -DARMCM33_DSP_FP -DPLATFORM_AP -mthumb -gdwarf-2 -mthumb-interwork)

set_property(GLOBAL PROPERTY CSTD)
set_compiler_property(PROPERTY nostdinc)

zephyr_sources(
  firmware_verification_table.S
  soc.c
  )

zephyr_sources_ifdef(CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_MPU_REGIONS mpu_regions.c)
zephyr_sources_ifdef(CONFIG_CACHE_MANAGEMENT cache.c)
zephyr_linker_sources(SECTIONS csk6001_private.ld)

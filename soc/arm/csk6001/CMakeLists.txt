# SPDX-License-Identifier: Apache-2.0

# zephyr_library_sources_ifdef(CONFIG_PLATFORM_SPECIFIC_INIT gcc/startup_ARMCM33.S)


zephyr_ld_options(-mthumb -gdwarf-2 -mapcs-frame -mthumb-interwork -static -Wl,-gc-sections --specs=nosys.specs)

zephyr_compile_options(-gdwarf-2 -g3 -Wall -Wno-unused -Wno-comment -MMD -ffunction-sections -fdata-sections -mthumb -gdwarf-2 -mapcs-frame -mthumb-interwork -DARMCM33_DSP_FP -DPLATFORM_AP -mthumb -gdwarf-2 -mthumb-interwork)

# set_compiler_property(PROPERTY cstd -std=gun11)
set_property(GLOBAL PROPERTY CSTD gnu11)
set_compiler_property(PROPERTY nostdinc)
# set_compiler_property(PROPERTY security_fortify _FORTIFY_SOURCE=0)
# set_property(GLOBAL PROPERTY CONFIG_FP16 0)

zephyr_sources(
  firmware_verification_table.S
  soc.c
  )

zephyr_sources_ifdef(CONFIG_HAS_EXTERNAL_CACHE cache.c)
zephyr_linker_sources(SECTIONS csk6001_private.ld)

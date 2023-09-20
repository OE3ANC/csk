if (CONFIG_MBEDTLS_BUILTIN)

if (CONFIG_CSK_MBEDTLS_HARDWARE_ALT)

zephyr_library()

set(MBEDPATH ${ZEPHYR_BASE}/../modules/crypto/mbedtls/include)
  zephyr_library_include_directories(${MBEDPATH})
  zephyr_include_directories_ifdef(CONFIG_CSK_MBEDTLS_HARDWARE_AES
    csk/csk_aes
    csk/configs
  )
  zephyr_library_sources_ifdef(CONFIG_CSK_MBEDTLS_HARDWARE_AES
    csk/csk_aes/csk_aes.c
    csk/csk_aes/csk_aes_xts.c
  )
endif()

endif()

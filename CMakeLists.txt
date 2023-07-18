set(CSK_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "CSK root directory")

add_subdirectory(drivers)
add_subdirectory(modules)

zephyr_include_directories(
        include
        include/drivers
        )

# override the default linker script,so that we can make more infomation in the zephyr.stat
set_property(TARGET bintools PROPERTY readelf_flag_headers -a)



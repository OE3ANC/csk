set(CSK_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "CSK root directory")

add_subdirectory(drivers)

zephyr_include_directories(
        include
        include/drivers
        )



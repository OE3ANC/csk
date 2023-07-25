# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

macro(csk_zephyr_ztest_amend input_path)
    set(__SOURCE_DIR ${input_path})
    while(__SOURCE_DIR)
        get_filename_component(__SOURCE_DIR ${__SOURCE_DIR} DIRECTORY)
        get_filename_component(LAST_DIR ${__SOURCE_DIR} NAME)
        if ("tests" STREQUAL LAST_DIR)
            get_filename_component(RELATIVE_PATH ${__SOURCE_DIR} DIRECTORY)
            get_filename_component(ABSOLUTE_PATH2 ${input_path} ABSOLUTE)
            file(RELATIVE_PATH RELATIVE_PATH2 ${RELATIVE_PATH} ${ABSOLUTE_PATH2})
            set(CSK_CURRENT_ZTEST_RELATIVE_PATH ${RELATIVE_PATH2})
            set(ZEPHYR_TESTS_DIR $ENV{ZEPHYR_BASE}/${CSK_CURRENT_ZTEST_RELATIVE_PATH})
            set(CONF_FILE ${ZEPHYR_TESTS_DIR}/prj.conf)
            break()
        endif()
    endwhile()
endmacro()

macro(csk_zephyr_ztest_directory_add)
    add_subdirectory(${ZEPHYR_TESTS_DIR} unused)
endmacro()

csk_zephyr_ztest_amend(${CMAKE_CURRENT_SOURCE_DIR})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(csk_pwm_api)
csk_zephyr_ztest_directory_add()


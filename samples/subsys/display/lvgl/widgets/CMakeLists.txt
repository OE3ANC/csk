# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(SUPPORT_BOARDS csk6_duomotai_devkit)
if(NOT ${BOARD} IN_LIST SUPPORT_BOARDS)
    message("Please choose one of the following boards:")
    foreach(item ${SUPPORT_BOARDS})
        message(STATUS ${item})
    endforeach()
    message(FATAL_ERROR "\nThe project does not support the ${BOARD}")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lvgl)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

set(LVGL_DIR ${ZEPHYR_LVGL_MODULE_DIR})
target_include_directories(app PRIVATE
    ${LVGL_DIR}/demos/
)

target_sources(app PRIVATE
    ${LVGL_DIR}/demos/widgets/assets/img_clothes.c
    ${LVGL_DIR}/demos/widgets/assets/img_demo_widgets_avatar.c
    ${LVGL_DIR}/demos/widgets/assets/img_lvgl_logo.c
    ${LVGL_DIR}/demos/widgets/lv_demo_widgets.c
)

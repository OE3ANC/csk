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
project(lvgl8)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

set(LVGL_DIR ${ZEPHYR_LVGL_MODULE_DIR})

target_include_directories(app PRIVATE
    ${LVGL_DIR}/demos/
)

target_sources(app PRIVATE
    ${LVGL_DIR}/demos/benchmark/lv_demo_benchmark.c
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_cogwheel_indexed16.c
    ${LVGL_DIR}/demos/benchmark/assets/lv_font_bechmark_montserrat_28_compr_az.c.c
    ${LVGL_DIR}/demos/benchmark/assets/lv_font_bechmark_montserrat_16_compr_az.c.c
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_cogwheel_argb.c
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_cogwheel_alpha16.c
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_cogwheel_rgb.c
    ${LVGL_DIR}/demos/benchmark/assets/lv_font_bechmark_montserrat_12_compr_az.c.c
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_cogwheel_chroma_keyed.c
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_cogwheel_rgb565a8.c
)


# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(CONF_FILE $ENV{ZEPHYR_BASE}/tests/drivers/pwm/pwm_api/prj.conf)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(csk_pwm_api)


add_subdirectory($ENV{ZEPHYR_BASE}/tests/drivers/pwm/pwm_api pwm_api)

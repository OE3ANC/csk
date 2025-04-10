# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_MPU6050 mpu6050)
add_subdirectory_ifdef(CONFIG_SC7A20H sc7a20h)
add_subdirectory_ifdef(CONFIG_QMI8658C qmi8658c)

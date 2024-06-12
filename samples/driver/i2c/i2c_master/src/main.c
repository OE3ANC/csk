/*
 * Copyright (c) 2023 Anhui(Shenzhen) Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/i2c.h>
//#include <i2c_slave_vdev.h>

//#include <zephyr/logging/log.h>
//LOG_MODULE_REGISTER(vdev_demo, LOG_LEVEL_INF);



#define I2C_DEV_LABEL "i2c_1" // 根据你的设备树修改
#define TARGET_DEVICE_ADDR 0x38      // I2C 从设备地址
#define TARGET_REG_ADDR 0x96      // I2C 从设备寄存器地址

void main(void)
{
    const struct device *i2c_dev;
	uint8_t i2c_reg_data_raw;
	uint8_t i2c_reg_data_modi;
    uint8_t data = 0x88; 

    int ret;

	i2c_dev = device_get_binding(I2C_DEV_LABEL);
    if (!i2c_dev) {
        printk("No I2C device found");
        return;
    }
    ret = i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER);
    if (ret < 0) {
        printk("i2c config failed\n");
        return -1;
    }
	
	ret = i2c_reg_read_byte(i2c_dev,TARGET_DEVICE_ADDR,TARGET_REG_ADDR,&i2c_reg_data_raw);
	if(ret < 0){
		printk("i2c_reg_read_byte failed!\n");
	}
	printk("i2c_reg_data_raw: %d\n",i2c_reg_data_raw);
	//k_sleep(K_MSEC(500));
	

	ret = i2c_reg_write_byte(i2c_dev,TARGET_DEVICE_ADDR,TARGET_REG_ADDR,data);
	if(ret < 0){
		printk("i2c_reg_write_byte failed!\n");
	}
	ret = i2c_reg_read_byte(i2c_dev,TARGET_DEVICE_ADDR,TARGET_REG_ADDR,&i2c_reg_data_modi);
	if(ret < 0){
		printk("i2c_reg_read_byte failed!\n");
	}
	printk("i2c_reg_data_modi: %d\n",i2c_reg_data_modi);

}


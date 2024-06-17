/*
 * Copyright (c) 2023 Anhui(Shenzhen) Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>


#define I2C1_NODE DT_INST(0, listenai_csk_i2c)
#define TARGET_DEVICE_ADDR 	0x38      	
#define TARGET_REG_ADDR 	0xA8     	

int main(void)
{
    const struct device *i2c_dev;
	uint8_t i2c_reg_data_raw;
	uint8_t i2c_reg_data_modify;
    uint8_t data = 0x99; 

    int ret;


	i2c_dev = DEVICE_DT_GET(I2C1_NODE);
    if (!i2c_dev) {
        printk("No I2C device found");
        return -1;
    }
    ret = i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER);
    if (ret < 0) {
        printk("i2c config failed\n");
        return -1;
    }
	
	ret = i2c_reg_read_byte(i2c_dev,TARGET_DEVICE_ADDR,TARGET_REG_ADDR,&i2c_reg_data_raw);
	if(ret < 0){
		printk("i2c_reg_read_byte failed!\n");
		return -1;
	}
	printk("i2c_reg_data_raw: %d\n",i2c_reg_data_raw);
	

	ret = i2c_reg_write_byte(i2c_dev,TARGET_DEVICE_ADDR,TARGET_REG_ADDR,data);
	if(ret < 0){
		printk("i2c_reg_write_byte failed!\n");
		return -1;
	}
	ret = i2c_reg_read_byte(i2c_dev,TARGET_DEVICE_ADDR,TARGET_REG_ADDR,&i2c_reg_data_modify);
	if(ret < 0){
		printk("i2c_reg_read_byte failed!\n");
		return -1;
	}
	printk("i2c_reg_data_modify: %d\n",i2c_reg_data_modify);
	
	return 0;
}


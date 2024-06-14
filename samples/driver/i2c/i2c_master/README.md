# 简介
演示如何使用 MCU 的 i2c 通信功能，通过 i2c 控制器向从设备的寄存器地址写数据和读数据。

# 构建步骤
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/i2c/i2c_master -p


# 功能说明
1.从设备树中获取 i2c 控制器设备
```i2c_dev = device_get_binding(I2C_DEV_LABEL);```

2.配置i2c控制器
```ret = i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER);```

3.读取寄存器原始数据
```ret = i2c_reg_read_byte(i2c_dev,TARGET_DEVICE_ADDR,TARGET_REG_ADDR,&i2c_reg_data_raw);```

4.写数据到寄存器
```ret = i2c_reg_write_byte(i2c_dev,TARGET_DEVICE_ADDR,TARGET_REG_ADDR,data);```

5.读取修改后的寄存器数据
```ret = i2c_reg_read_byte(i2c_dev,TARGET_DEVICE_ADDR,TARGET_REG_ADDR,&i2c_reg_data_modi);```

# 运行
按下开发板复位按钮，运行程序，观察串口信息。
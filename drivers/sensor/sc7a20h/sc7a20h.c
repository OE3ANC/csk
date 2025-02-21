#define DT_DRV_COMPAT listenai_sc7a20h

#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "sc7a20h.h"

LOG_MODULE_REGISTER(SC7A20H_CSK6, CONFIG_SENSOR_LOG_LEVEL);

static void sc7a20h_convert_accel(struct sensor_value *val, int16_t raw_val)
{
    int64_t conv_val;
    // 将原始加速度值转换为标准加速度值
    conv_val = ((int64_t)raw_val * SENSOR_G) / 16384;
    //val1 存储整数部分，val2 存储小数部分
    val->val1 = conv_val / 1000000;
    val->val2 = conv_val % 1000000;
}
//chan为读取哪个轴的值 val为存储读取值的指针
static int sc7a20h_channel_get(const struct device *dev,
                               enum sensor_channel chan,
                               struct sensor_value *val)
{
    struct sc7a20h_data *drv_data = dev->data;

    switch (chan) {
        //zephyr 的sensor驱动有
    case SENSOR_CHAN_ACCEL_XYZ:
        sc7a20h_convert_accel(val, drv_data->accel_x);
        sc7a20h_convert_accel(val + 1, drv_data->accel_y);
        sc7a20h_convert_accel(val + 2, drv_data->accel_z);
        break;
    case SENSOR_CHAN_ACCEL_X:
        sc7a20h_convert_accel(val, drv_data->accel_x);
        break;
    case SENSOR_CHAN_ACCEL_Y:
        sc7a20h_convert_accel(val, drv_data->accel_y);
        break;
    case SENSOR_CHAN_ACCEL_Z:
        sc7a20h_convert_accel(val, drv_data->accel_z);
        break;
    default:
        return -ENOTSUP;
    }

    return 0;
}

static int sc7a20h_sample_fetch(const struct device *dev,
                                enum sensor_channel chan)
{
    struct sc7a20h_data *drv_data = dev->data;
    const struct sc7a20h_config *cfg = dev->config;
    int16_t buf[3];
    //从寄存器读取数据 读取到原始数据 转换为CPU字节序 然后存储到drv_data中 就是存储在设备数据结构中
    if (i2c_burst_read_dt(&cfg->i2c, SC7A20H_REG_DATA_START, (uint8_t *)buf,
                          6) < 0) {
        LOG_ERR("Failed to read data sample.");
        return -EIO;
    }

    drv_data->accel_x = sys_le16_to_cpu(buf[0]);
    drv_data->accel_y = sys_le16_to_cpu(buf[1]);
    drv_data->accel_z = sys_le16_to_cpu(buf[2]);

    return 0;
}

static const struct sensor_driver_api sc7a20h_driver_api = {
#if CONFIG_SC7A20H_TRIGGER
//1.传感器如果中断触发 就进入sc7a20h_trigger.c  里面线程cb 和 gpio的cb
    .trigger_set = sc7a20h_trigger_set,
#endif
    .sample_fetch = sc7a20h_sample_fetch,
    .channel_get = sc7a20h_channel_get,
};

int sc7a20h_init(const struct device *dev)
{
    //三轴加速度
    struct sc7a20h_data *drv_data = dev->data;
    //I2C设备配置
    const struct sc7a20h_config *cfg = dev->config;
    uint8_t id;

    if (!device_is_ready(cfg->i2c.bus)) {
        //这里要加入处理I2C设备未准备好的代码 LOG也行
        LOG_ERR("Bus device is not ready");
        return -ENODEV;
    }

    /* check chip ID */
    //IICinit 对OXOF的寄存器进行读取 确认读取的ID是否与预期值匹配 看看芯片手册
    if (i2c_reg_read_byte_dt(&cfg->i2c, SC7A20H_REG_CHIP_ID, &id) < 0) {
        LOG_ERR("Failed to read chip ID.");
        return -EIO;
    }

    if (id != SC7A20H_CHIP_ID) {
        LOG_ERR("Invalid chip ID.");
        return -EINVAL;
    }

    /* configure control registers 
        CTRL1寄存器（0x20）配置为0x57：启用XYZ轴、设置100Hz输出数据率
        CTRL4寄存器（0x23）配置为0x00：禁用中断、设置±2g量程
    */
    if (i2c_reg_write_byte_dt(&cfg->i2c, SC7A20H_REG_CTRL1, 0x57) < 0) {
        LOG_ERR("Failed to configure control register 1.");
        return -EIO;
    }

    if (i2c_reg_write_byte_dt(&cfg->i2c, SC7A20H_REG_CTRL4, 0x00) < 0) {
        LOG_ERR("Failed to configure control register 4.");
        return -EIO;
    }

#ifdef CONFIG_SC7A20H_TRIGGER
    if (cfg->int_gpio.port) {
        if (sc7a20h_init_interrupt(dev) < 0) {
            LOG_DBG("Failed to initialize interrupts.");
            return -EIO;
        }
    }
#endif

    return 0;
}
//实例化宏
#define SC7A20H_DEFINE(inst)                                   \
    static struct sc7a20h_data sc7a20h_data_##inst;            \
                                                               \
    static const struct sc7a20h_config sc7a20h_config_##inst = {\
        .i2c = I2C_DT_SPEC_INST_GET(inst),                     \
        IF_ENABLED(CONFIG_SC7A20H_TRIGGER,                     \
                   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, { 0 }),)) \
    };                                                         \
                                                \
    SENSOR_DEVICE_DT_INST_DEFINE(inst, sc7a20h_init, NULL,     \
                                 &sc7a20h_data_##inst, &sc7a20h_config_##inst, \
                                 POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
                                 &sc7a20h_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SC7A20H_DEFINE)

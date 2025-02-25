#define DT_DRV_COMPAT listenai_sc7a20h

#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "sc7a20h.h"

LOG_MODULE_REGISTER(SC7A20H_CSK6, CONFIG_SENSOR_LOG_LEVEL);

// 添加比例因子定义
#define SENSITIVITY_2G    1024  // LSB/g
#define SENSITIVITY_4G    512   // LSB/g
#define SENSITIVITY_8G    256   // LSB/g
#define SENSITIVITY_16G   128   // LSB/g

static void sc7a20h_convert_accel(struct sensor_value *val, int16_t raw_val)
{
    // 使用8g量程的灵敏度
    int64_t conv_val;
    conv_val = ((int64_t)raw_val * SENSOR_G) / SENSITIVITY_8G;
    val->val1 = conv_val / 1000000;
    val->val2 = conv_val % 1000000;
}

static int sc7a20h_channel_get(const struct device *dev,
                               enum sensor_channel chan,
                               struct sensor_value *val)
{
    struct sc7a20h_data *drv_data = dev->data;

    // 确保数据有效
    if (!drv_data) {
        return -EINVAL;
    }

    switch (chan) {
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
    uint8_t buf[6];
    int ret;

    // 确保数据有效
    if (!drv_data || !cfg) {
        return -EINVAL;
    }

    // 先读取状态寄存器确保数据就绪
    uint8_t status;
    ret = i2c_reg_read_byte_dt(&cfg->i2c, SC7A20H_REG_STATUS, &status);
    if (ret < 0) {
        LOG_ERR("Failed to read status register");
        return ret;
    }

    if (!(status & 0x08)) {  // 检查数据就绪位
        return -EBUSY;
    }

    // 使用多字节读取确保数据一致性
    ret = i2c_burst_read_dt(&cfg->i2c, SC7A20H_REG_OUT_X_L, buf, 6);
    if (ret < 0) {
        LOG_ERR("Failed to read acceleration data");
        return ret;
    }

    // 组合数据，注意字节序
    drv_data->accel_x = (int16_t)((buf[1] << 8) | buf[0]);
    drv_data->accel_y = (int16_t)((buf[3] << 8) | buf[2]);
    drv_data->accel_z = (int16_t)((buf[5] << 8) | buf[4]);

    LOG_DBG("x: %d, y: %d, z: %d",
            drv_data->accel_x,
            drv_data->accel_y,
            drv_data->accel_z);

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
    int ret;

    if (!device_is_ready(cfg->i2c.bus)) {
        //这里要加入处理I2C设备未准备好的代码 LOG也行
        LOG_ERR("Bus device is not ready");
        return -ENODEV;
    }

    // 初始化 drv_data
    drv_data->accel_x = 0;
    drv_data->accel_y = 0;
    drv_data->accel_z = 0;

    /* check chip ID */
    //IICinit 对OXOF的寄存器进行读取 确认读取的ID是否与预期值匹配 看看芯片手册
    ret = i2c_reg_read_byte_dt(&cfg->i2c, SC7A20H_REG_CHIP_ID, &id);
    if (ret < 0) {
        LOG_ERR("Failed to read chip ID");
        return ret;
    }

    if (id != SC7A20H_CHIP_ID) {
        LOG_ERR("Invalid chip ID: 0x%02x", id);
        return -EINVAL;
    }

    // 复位芯片
    if (i2c_reg_write_byte_dt(&cfg->i2c, SC7A20H_REG_CTRL2, 0x40) < 0) {
        return -EIO;
    }
    k_msleep(10);  // 等待复位完成

    // 配置CTRL1：使能XYZ轴，设置50Hz采样率
    if (i2c_reg_write_byte_dt(&cfg->i2c, SC7A20H_REG_CTRL1, 
        SC7A20H_CTRL1_XEN | SC7A20H_CTRL1_YEN | SC7A20H_CTRL1_ZEN | 
        SC7A20H_CTRL1_ODR_50HZ) < 0) {
        return -EIO;
    }

    // 配置CTRL4：设置±2g量程，高分辨率模式
    if (i2c_reg_write_byte_dt(&cfg->i2c, SC7A20H_REG_CTRL4, 
        SC7A20H_CTRL4_FS_2G | BIT(3)) < 0) {
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

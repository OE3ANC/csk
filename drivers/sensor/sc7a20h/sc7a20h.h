#ifndef ZEPHYR_DRIVERS_SENSOR_SC7A20H_SC7A20H_H_
#define ZEPHYR_DRIVERS_SENSOR_SC7A20H_SC7A20H_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#define SC7A20H_REG_CHIP_ID       0x0F
#define SC7A20H_CHIP_ID           0x11

#define SC7A20H_REG_CTRL1         0x20 //设置xyz轴是否使能
#define SC7A20H_REG_CTRL2         0x21
#define SC7A20H_REG_CTRL3         0x22
#define SC7A20H_REG_CTRL4         0x23
#define SC7A20H_REG_CTRL5         0x24
#define SC7A20H_REG_CTRL6         0x25

#define SC7A20H_REG_INT1_CFG      0x30
#define SC7A20H_REG_INT1_SRC      0x31
#define SC7A20H_REG_STATUS        0x27
#define SC7A20H_REG_OUT_X_L       0x28
#define SC7A20H_REG_OUT_X_H       0x29
#define SC7A20H_REG_OUT_Y_L       0x2A
#define SC7A20H_REG_OUT_Y_H       0x2B
#define SC7A20H_REG_OUT_Z_L       0x2C
#define SC7A20H_REG_OUT_Z_H       0x2D

#define SC7A20H_REG_DATA_START    0x28

// CTRL1寄存器位定义
#define SC7A20H_CTRL1_XEN        BIT(0)  // X轴使能
#define SC7A20H_CTRL1_YEN        BIT(1)  // Y轴使能
#define SC7A20H_CTRL1_ZEN        BIT(2)  // Z轴使能
#define SC7A20H_CTRL1_ODR_MASK   (BIT(4) | BIT(5) | BIT(6) | BIT(7))
#define SC7A20H_CTRL1_ODR_10HZ   (0x2 << 4)
#define SC7A20H_CTRL1_ODR_50HZ   (0x4 << 4)
#define SC7A20H_CTRL1_ODR_100HZ  (0x5 << 4)
#define SC7A20H_CTRL1_ODR_200HZ  (0x6 << 4)

// CTRL4寄存器位定义
#define SC7A20H_CTRL4_FS_MASK    (BIT(4) | BIT(5))
#define SC7A20H_CTRL4_FS_2G      (0x0 << 4)
#define SC7A20H_CTRL4_FS_4G      (0x1 << 4)
#define SC7A20H_CTRL4_FS_8G      (0x2 << 4)
#define SC7A20H_CTRL4_FS_16G     (0x3 << 4)

// 添加灵敏度定义
#define SC7A20H_SENSITIVITY_2G    1024  // LSB/g
#define SC7A20H_SENSITIVITY_4G    512   // LSB/g
#define SC7A20H_SENSITIVITY_8G    256   // LSB/g
#define SC7A20H_SENSITIVITY_16G   128   // LSB/g

struct sc7a20h_data {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;

    uint16_t sensitivity;  // 添加灵敏度配置
    const struct device *dev;  // 添加设备指针

#ifdef CONFIG_SC7A20H_TRIGGER
    struct gpio_callback gpio_cb;

    const struct sensor_trigger *data_ready_trigger;
    sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_SC7A20H_TRIGGER_OWN_THREAD)
    K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_SC7A20H_THREAD_STACK_SIZE);
    struct k_thread thread;
    struct k_sem gpio_sem;
#elif defined(CONFIG_SC7A20H_TRIGGER_GLOBAL_THREAD)
    struct k_work work;
#endif

#endif /* CONFIG_SC7A20H_TRIGGER */
};

struct sc7a20h_config {
    struct i2c_dt_spec i2c;
    
#ifdef CONFIG_SC7A20H_TRIGGER
    struct gpio_dt_spec int_gpio;
#endif /* CONFIG_SC7A20H_TRIGGER */
};

#ifdef CONFIG_SC7A20H_TRIGGER
int sc7a20h_trigger_set(const struct device *dev,
                        const struct sensor_trigger *trig,
                        sensor_trigger_handler_t handler);

int sc7a20h_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_SC7A20H_SC7A20H_H_ */

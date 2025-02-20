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

#define SC7A20H_REG_CTRL1         0x20
#define SC7A20H_REG_CTRL4         0x23
#define SC7A20H_REG_INT1_CFG      0x30
#define SC7A20H_REG_INT1_SRC      0x31
#define SC7A20H_REG_DATA_START    0x28

struct sc7a20h_data {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;

#ifdef CONFIG_SC7A20H_TRIGGER
    const struct device *dev;
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

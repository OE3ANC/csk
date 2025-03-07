#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "sc7a20h.h"


LOG_MODULE_DECLARE(SC7A20H_CSK6, CONFIG_LOG_DEFAULT_LEVEL);

int sc7a20h_trigger_set(const struct device *dev,
                        const struct sensor_trigger *trig,
                        sensor_trigger_handler_t handler)
{
    struct sc7a20h_data *drv_data = dev->data;
    const struct sc7a20h_config *cfg = dev->config;

    if (!cfg->int_gpio.port) {
        return -ENOTSUP;
    }

    if (trig->type != SENSOR_TRIG_DATA_READY) {
        return -ENOTSUP;
    }

    gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

    drv_data->data_ready_handler = handler;
    if (handler == NULL) {
        return 0;
    }

    drv_data->data_ready_trigger = trig;

    gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
                                    GPIO_INT_EDGE_TO_ACTIVE);

    return 0;
}

static void sc7a20h_gpio_callback(const struct device *dev,
                                  struct gpio_callback *cb, uint32_t pins)
{
    struct sc7a20h_data *drv_data =
        CONTAINER_OF(cb, struct sc7a20h_data, gpio_cb);//使用这个宏回调结构体指针cb获取sc7a20h_data结构体指针drv_data
    const struct sc7a20h_config *cfg = drv_data->dev->config;

    ARG_UNUSED(pins);

    gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_SC7A20H_TRIGGER_OWN_THREAD)
    k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_SC7A20H_TRIGGER_GLOBAL_THREAD)
    k_work_submit(&drv_data->work);
#endif
}

static void sc7a20h_thread_cb(const struct device *dev)
{
    struct sc7a20h_data *drv_data = dev->data;
    const struct sc7a20h_config *cfg = dev->config;

    if (drv_data->data_ready_handler != NULL) {
        drv_data->data_ready_handler(dev,
                                     drv_data->data_ready_trigger);
    }

    gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
                                    GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_SC7A20H_TRIGGER_OWN_THREAD
static void sc7a20h_thread(struct sc7a20h_data *drv_data)
{
    while (1) {
        k_sem_take(&drv_data->gpio_sem, K_FOREVER);
        sc7a20h_thread_cb(drv_data->dev);
    }
}
#endif

#ifdef CONFIG_SC7A20H_TRIGGER_GLOBAL_THREAD
static void sc7a20h_work_cb(struct k_work *work)
{
    struct sc7a20h_data *drv_data =
        CONTAINER_OF(work, struct sc7a20h_data, work);

    if (drv_data->data_ready_handler) {
        drv_data->data_ready_handler(drv_data->dev,
                                   drv_data->data_ready_trigger);
    }
}
#endif

int sc7a20h_init_interrupt(const struct device *dev)
{
    struct sc7a20h_data *drv_data = dev->data;
    const struct sc7a20h_config *cfg = dev->config;

    if (!device_is_ready(cfg->int_gpio.port)) {
        LOG_ERR("GPIO device not ready");
        return -ENODEV;
    }

    drv_data->dev = dev;

    gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT | GPIO_PULL_UP);

    gpio_init_callback(&drv_data->gpio_cb,
                       sc7a20h_gpio_callback,
                       BIT(cfg->int_gpio.pin));

    if (gpio_add_callback(cfg->int_gpio.port, &drv_data->gpio_cb) < 0) {
        LOG_ERR("Failed to set gpio callback");
        return -EIO;
    }

    /* enable data ready interrupt */
    if (i2c_reg_write_byte_dt(&cfg->i2c, SC7A20H_REG_INT1_CFG,
                              0x01) < 0) {
        LOG_ERR("Failed to enable data ready interrupt.");
        return -EIO;
    }

#if defined(CONFIG_SC7A20H_TRIGGER_OWN_THREAD)
    k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

    k_thread_create(&drv_data->thread, drv_data->thread_stack,
                    CONFIG_SC7A20H_THREAD_STACK_SIZE,
                    (k_thread_entry_t)sc7a20h_thread, drv_data,
                    NULL, NULL, K_PRIO_COOP(CONFIG_SC7A20H_THREAD_PRIORITY),
                    0, K_NO_WAIT);
#elif defined(CONFIG_SC7A20H_TRIGGER_GLOBAL_THREAD)
    drv_data->work.handler = sc7a20h_work_cb;
#endif

    gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_FALLING);

    return 0;
}

/*
 * Copyright (c) 2025 Anhui Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "qmi8658c.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(QMI8658C_CSK6, CONFIG_SENSOR_LOG_LEVEL);

int qmi8658c_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct qmi8658c_data *drv_data = dev->data;
	const struct qmi8658c_config *cfg = dev->config;

	if (!cfg->int_gpio.port) {
		return -ENOTSUP;
	}

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = trig;

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}

static void qmi8658c_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct qmi8658c_data *drv_data = CONTAINER_OF(cb, struct qmi8658c_data, gpio_cb);

#if defined(CONFIG_QMI8658C_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_QMI8658C_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

#ifdef CONFIG_QMI8658C_TRIGGER_OWN_THREAD
static void qmi8658c_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct qmi8658c_data *drv_data = p1;

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);

		if (drv_data->data_ready_handler) {
			drv_data->data_ready_handler(drv_data->dev, drv_data->data_ready_trigger);
		}
	}
}
#endif

#ifdef CONFIG_QMI8658C_TRIGGER_GLOBAL_THREAD
static void qmi8658c_work_cb(struct k_work *work)
{
	struct qmi8658c_data *drv_data = CONTAINER_OF(work, struct qmi8658c_data, work);

	if (drv_data->data_ready_handler) {
		drv_data->data_ready_handler(drv_data->dev, drv_data->data_ready_trigger);
	}
}
#endif

int qmi8658c_init_interrupt(const struct device *dev)
{
	struct qmi8658c_data *drv_data = dev->data;
	const struct qmi8658c_config *cfg = dev->config;

	if (!device_is_ready(cfg->int_gpio.port)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT | GPIO_PULL_UP);
	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_FALLING);

	gpio_init_callback(&drv_data->gpio_cb, qmi8658c_gpio_callback, BIT(cfg->int_gpio.pin));

	if (gpio_add_callback(cfg->int_gpio.port, &drv_data->gpio_cb) < 0) {
		LOG_ERR("Failed to add gpio callback");
		return -EIO;
	}

	drv_data->dev = dev;

#if defined(CONFIG_QMI8658C_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_QMI8658C_THREAD_STACK_SIZE, (k_thread_entry_t)qmi8658c_thread,
			drv_data, NULL, NULL, K_PRIO_COOP(CONFIG_QMI8658C_THREAD_PRIORITY), 0,
			K_NO_WAIT);
#elif defined(CONFIG_QMI8658C_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = qmi8658c_work_cb;
#endif

	return 0;
}

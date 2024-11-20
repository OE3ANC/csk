/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys/util_internal.h"
#include <stdbool.h>
#define DT_DRV_COMPAT listenai_mipi_dbi_spi

#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mipi_dbi_spi_csk, CONFIG_MIPI_DBI_LOG_LEVEL);

#include "display_csk6_lcd.h"


struct mipi_dbi_spi_config {
	/* SPI hardware used to send data */
	const struct device *spi_dev;
	/* Command/Data gpio */
	const struct gpio_dt_spec cmd_data;
	/* Reset GPIO */
	const struct gpio_dt_spec reset;
	/* dma channel */
	const int dma_ch;

	const int rotate;

	const uint8_t ca_set;
	const uint8_t ra_set;
};

struct mipi_dbi_spi_data {
	/* Used for 3 wire mode */
	uint16_t spi_byte;
	struct k_mutex lock;
	void *csk6_priv;
	uint8_t ca_set_datas[4];
	uint8_t ra_set_datas[4];
};

/* Expands to 1 if the node does not have the `write-only` property */
#define _WRITE_ONLY_ABSENT(n) (!DT_INST_PROP(n, write_only)) |

/* This macro will evaluate to 1 if any of the nodes with zephyr,mipi-dbi-spi
 * lack a `write-only` property. The intention here is to allow the entire
 * command_read function to be optimized out when it is not needed.
 */
#define MIPI_DBI_SPI_READ_REQUIRED DT_INST_FOREACH_STATUS_OKAY(_WRITE_ONLY_ABSENT) 0
uint32_t var = MIPI_DBI_SPI_READ_REQUIRED;

/* In Type C mode 1 MIPI BIT communication, the 9th bit of the word
 * (first bit sent in each word) indicates if the word is a command or
 * data. Typically 0 indicates a command and 1 indicates data, but some
 * displays may vary.
 */
#define MIPI_DBI_DC_BIT BIT(9)

static int mipi_dbi_spi_command_write(const struct device *dev,
				      const struct mipi_dbi_config *dbi_config,
				      uint8_t cmd, const uint8_t *data_buf,
				      size_t len)
{
	const struct mipi_dbi_spi_config *config = dev->config;
	struct mipi_dbi_spi_data *data = dev->data;
	int ret = 0;

	if (dbi_config->mode != MIPI_DBI_MODE_SPI_4WIRE) {
		return -ENOTSUP;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	/* Set CD pin low for command */
	gpio_pin_set_dt(&config->cmd_data, 0);
	ret = display_csk6_lcd_cmd_datas_send(data->csk6_priv, &cmd, 1);
	if (ret < 0) {
		goto out;
	}

	if (len > 0) {
		/* Set CD pin high for data */
		gpio_pin_set_dt(&config->cmd_data, 1);
		ret = display_csk6_lcd_cmd_datas_send(data->csk6_priv, data_buf, len);

		if (ret < 0) {
			goto out;
		}
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int mipi_dbi_spi_write_display(const struct device *dev,
				      const struct mipi_dbi_config *dbi_config,
				      const uint8_t *framebuf,
				      struct display_buffer_descriptor *desc,
				      enum display_pixel_format pixfmt)
{
	ARG_UNUSED(pixfmt);

	const struct mipi_dbi_spi_config *config = dev->config;
	struct mipi_dbi_spi_data *data = dev->data;
	int ret = 0;

	if (dbi_config->mode != MIPI_DBI_MODE_SPI_4WIRE) {
		return -ENOTSUP;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}
	gpio_pin_set_dt(&config->cmd_data, 1);

	if (config->rotate == 90) {
		ret = display_csk6_lcd_display_datas_send(data->csk6_priv, framebuf, desc->width,
							  desc->height, 1);
	} else if (config->rotate == 0) {
		ret = display_csk6_lcd_display_datas_send(data->csk6_priv, framebuf, desc->height,
							  desc->width, 0);
	} else {
		LOG_ERR("unsupported rotation value: %d", config->rotate);
		ret = -ENOTSUP;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static inline bool mipi_dbi_has_pin(const struct gpio_dt_spec *spec)
{
	return spec->port != NULL;
}

static int mipi_dbi_spi_reset(const struct device *dev, uint32_t delay)
{
	const struct mipi_dbi_spi_config *config = dev->config;
	int ret;

	if (!mipi_dbi_has_pin(&config->reset)) {
		return -ENOTSUP;
	}

	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret < 0) {
		return ret;
	}
	k_msleep(delay);
	return gpio_pin_set_dt(&config->reset, 0);
}

static int mipi_dbi_spi_release(const struct device *dev,
				const struct mipi_dbi_config *dbi_config)
{
	const struct mipi_dbi_spi_config *config = dev->config;

	return spi_release(config->spi_dev, &dbi_config->config);
}

static int mipi_dbi_spi_init(const struct device *dev)
{
	const struct mipi_dbi_spi_config *config = dev->config;
	struct mipi_dbi_spi_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->spi_dev)) {
		LOG_ERR("SPI device is not ready");
		return -ENODEV;
	}

	data->csk6_priv = display_csk6_lcd_init(config->spi_dev->name, config->dma_ch);
	if (data->csk6_priv == NULL) {
		LOG_ERR("SPI device init failed");
		return -ENODEV;
	}

	if (mipi_dbi_has_pin(&config->cmd_data)) {
		if (!gpio_is_ready_dt(&config->cmd_data)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->cmd_data, GPIO_OUTPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure command/data GPIO (%d)", ret);
			return ret;
		}
	}

	if (mipi_dbi_has_pin(&config->reset)) {
		if (!gpio_is_ready_dt(&config->reset)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", ret);
			return ret;
		}
	}

	k_mutex_init(&data->lock);

	return 0;
}

static struct mipi_dbi_driver_api mipi_dbi_spi_driver_api = {
	.reset = mipi_dbi_spi_reset,
	.command_write = mipi_dbi_spi_command_write,
	.write_display = mipi_dbi_spi_write_display,
	.release = mipi_dbi_spi_release,
};

#define MIPI_DBI_SPI_INIT(n)							\
	static const struct mipi_dbi_spi_config					\
	    mipi_dbi_spi_config_##n = {						\
		    .spi_dev = DEVICE_DT_GET(					\
				    DT_INST_PHANDLE(n, spi_dev)),		\
		    .cmd_data = GPIO_DT_SPEC_INST_GET_OR(n, dc_gpios, {}),	\
		    .reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {}),	\
			.dma_ch = DT_INST_PROP_OR(n, dma_ch, 0),	\
			.rotate = DT_INST_PROP_OR(n, rotate, 0),	\
	};									\
	static struct mipi_dbi_spi_data mipi_dbi_spi_data_##n;			\
										\
	DEVICE_DT_INST_DEFINE(n, mipi_dbi_spi_init, NULL,			\
			&mipi_dbi_spi_data_##n,					\
			&mipi_dbi_spi_config_##n,				\
			POST_KERNEL,						\
			CONFIG_MIPI_DBI_INIT_PRIORITY,				\
			&mipi_dbi_spi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MIPI_DBI_SPI_INIT)

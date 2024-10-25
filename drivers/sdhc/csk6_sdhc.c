/*
 *  Copyright (c) 2021 listenai Systems (anhui) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_csk6_sdhc

#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sd/sd_spec.h>

#include <soc.h>

LOG_MODULE_REGISTER(csk6_sdhc, CONFIG_SDHC_LOG_LEVEL);

#include "ftsdc021.h"

struct csk6_sdhc_config {
	ftsdc021_reg *base;
	const struct gpio_dt_spec pwr_gpio;
	const struct gpio_dt_spec detect_gpio;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_config_func)(const struct device *dev);
};

struct csk6_sdhc_data {
	const struct device *dev;
	struct sdhc_host_props props;
	bool card_present;
	struct k_sem transfer_sem;
	volatile uint32_t transfer_status;
	sdhc_handle_t transfer_handle;
	struct sdhc_io host_io;
	struct k_mutex access_mutex;
	sdhc_interrupt_cb_t sdhc_cb;
	struct gpio_callback cd_callback;
	void *sdhc_cb_user_data;
	uint8_t sdhc_rx_dummy[128] __aligned(32);
#ifdef CONFIG_CSK6_SDHC_DMA_SUPPORT
	uint32_t *sdhc_dma_descriptor; /* ADMA descriptor table (noncachable) */
	uint32_t dma_descriptor_len;   /* DMA descriptor table length in words */
#endif
};

static void card_detect_gpio_cb(const struct device *port, struct gpio_callback *cb,
				gpio_port_pins_t pins)
{
	struct csk6_sdhc_data *data = CONTAINER_OF(cb, struct csk6_sdhc_data, cd_callback);
	const struct device *dev = data->dev;
	const struct csk6_sdhc_config *cfg = dev->config;

	if (data->sdhc_cb) {
		if (gpio_pin_get_dt(&cfg->detect_gpio)) {
			data->sdhc_cb(dev, SDHC_INT_INSERTED, data->sdhc_cb_user_data);
		} else {
			data->sdhc_cb(dev, SDHC_INT_REMOVED, data->sdhc_cb_user_data);
		}
	}
}

static int csk6_sdhc_reset(const struct device *dev)
{
	const struct csk6_sdhc_config *cfg = dev->config;
	int i;

	cfg->base->SoftRst |= (SDHCI_SOFTRST_CMD | SDHCI_SOFTRST_DAT);

	for (i = 0; i < 100; i++) {
		if ((cfg->base->SoftRst & (SDHCI_SOFTRST_CMD | SDHCI_SOFTRST_DAT)) == 0)
			return 0;
		k_msleep(1);
	}

	return -1;
}

/*
 * Set SDHC io properties
 */
static int csk6_sdhc_set_io(const struct device *dev, struct sdhc_io *ios)
{
	const struct csk6_sdhc_config *cfg = dev->config;
	struct csk6_sdhc_data *data = dev->data;
	struct sdhc_io *host_io = &data->host_io;

	LOG_DBG("SDHC I/O: bus width %d, clock %dHz, card power %s, voltage %s", ios->bus_width,
		ios->clock, ios->power_mode == SDHC_POWER_ON ? "ON" : "OFF",
		ios->signal_voltage == SD_VOL_1_8_V ? "1.8V" : "3.3V");

	/* Set host clock */
	if (ios->clock && (ios->clock > data->props.f_max || ios->clock < data->props.f_min)) {
		LOG_ERR("SDHC I/O: Set invalid clock %d (min:%d -- max:%d)", ios->clock,
			data->props.f_min, data->props.f_max);
		return -EINVAL;
	}

	if (host_io->clock != ios->clock) {
		// TODO
		host_io->clock = ios->clock;
	}

	/* Set bus width */
	if (host_io->bus_width != ios->bus_width) {
		switch (ios->bus_width) {
		case SDHC_BUS_WIDTH1BIT:
			cfg->base->HCReg &= ~SDHCI_HC_BUS_WIDTH_4BIT;
			break;
		case SDHC_BUS_WIDTH4BIT:
			cfg->base->HCReg |= SDHCI_HC_BUS_WIDTH_4BIT;
			break;
		default:
			return -ENOTSUP;
		}
		host_io->bus_width = ios->bus_width;
	}

	/* Set host signal voltage */
	if (ios->signal_voltage != host_io->signal_voltage) {
		switch (ios->signal_voltage) {
		case SD_VOL_3_3_V:
			cfg->base->HCReg &= ~(7 << 1);
			cfg->base->HCReg |= SDHCI_POWER_330;
			break;
		case SD_VOL_3_0_V:
			cfg->base->HCReg &= ~(7 << 1);
			cfg->base->HCReg |= SDHCI_POWER_300;
			break;
		case SD_VOL_1_8_V:
			cfg->base->HCReg &= ~(7 << 1);
			cfg->base->HCReg |= SDHCI_POWER_180;
			break;
		default:
			return -ENOTSUP;
		}
		/* Save new host voltage */
		host_io->signal_voltage = ios->signal_voltage;
	}

	/* Set card power */
	if ((host_io->power_mode != ios->power_mode) && (cfg->pwr_gpio.port)) {
		if (ios->power_mode == SDHC_POWER_OFF) {
			gpio_pin_set_dt(&cfg->pwr_gpio, 0);
		} else if (ios->power_mode == SDHC_POWER_ON) {
			gpio_pin_set_dt(&cfg->pwr_gpio, 1);
		}
		host_io->power_mode = ios->power_mode;
	}

	/* Set I/O timing */
	if (host_io->timing != ios->timing) {
		switch (ios->timing) {
		case SDHC_TIMING_LEGACY:
		case SDHC_TIMING_HS:
			break;
		default:
			return -ENOTSUP;
		}
		host_io->timing = ios->timing;
	}

	return 0;
}

/*
 * Return 0 if card is not busy, 1 if it is
 */
static int csk6_sdhc_card_busy(const struct device *dev)
{
	const struct csk6_sdhc_config *cfg = dev->config;

	return 0;
}

/*
 * Execute card tuning
 */
static int csk6_sdhc_execute_tuning(const struct device *dev)
{
	const struct csk6_sdhc_config *cfg = dev->config;
	struct csk6_sdhc_data *dev_data = dev->data;

	return 0;
}

/*
 * Send CMD or CMD/DATA via SDHC
 */
static int csk6_sdhc_request(const struct device *dev, struct sdhc_command *cmd,
			     struct sdhc_data *data)
{
	const struct csk6_sdhc_config *cfg = dev->config;
	struct csk6_sdhc_data *dev_data = dev->data;

	return 0;
}

/*
 * Get card presence
 */
static int csk6_sdhc_get_card_present(const struct device *dev)
{
	const struct csk6_sdhc_config *cfg = dev->config;
	struct csk6_sdhc_data *data = dev->data;

	return ((int)data->card_present);
}

/*
 * Get host properties
 */
static int csk6_sdhc_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	struct csk6_sdhc_data *data = dev->data;

	memcpy(props, &data->props, sizeof(struct sdhc_host_props));
	return 0;
}

/*
 * Enable SDHC card interrupt
 */
static int csk6_sdhc_enable_interrupt(const struct device *dev, sdhc_interrupt_cb_t callback,
				      int sources, void *user_data)
{
	const struct csk6_sdhc_config *cfg = dev->config;
	struct csk6_sdhc_data *data = dev->data;
	int ret;

	return 0;
}

static int csk6_sdhc_disable_interrupt(const struct device *dev, int sources)
{
	const struct csk6_sdhc_config *cfg = dev->config;
	struct csk6_sdhc_data *data = dev->data;
	int ret;

	return 0;
}

static int csk6_sdhc_isr(const struct device *dev)
{
	const struct csk6_sdhc_config *cfg = dev->config;
	struct csk6_sdhc_data *data = dev->data;

	return 0;
}

/*
 * Perform early system init for SDHC
 */
static int csk6_sdhc_init(const struct device *dev)
{
	const struct csk6_sdhc_config *cfg = dev->config;
	struct csk6_sdhc_data *data = dev->data;
	int ret;

	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	if (cfg->pwr_gpio.port) {
		ret = gpio_pin_configure_dt(&cfg->pwr_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			return ret;
		}
	}

	if (cfg->detect_gpio.port) {
		ret = gpio_pin_configure_dt(&cfg->detect_gpio, GPIO_INPUT);
		if (ret) {
			return ret;
		}
		gpio_init_callback(&data->cd_callback, card_detect_gpio_cb,
				   BIT(cfg->detect_gpio.pin));
		ret = gpio_add_callback_dt(&cfg->detect_gpio, &data->cd_callback);
		if (ret) {
			return ret;
		}
	}

	cfg->irq_config_func(dev);

	return 0;
}

static const struct sdhc_driver_api sdhc_api = {
	.reset = csk6_sdhc_reset,
	.request = csk6_sdhc_request,
	.set_io = csk6_sdhc_set_io,
	.get_card_present = csk6_sdhc_get_card_present,
	.execute_tuning = csk6_sdhc_execute_tuning,
	.card_busy = csk6_sdhc_card_busy,
	.get_host_props = csk6_sdhc_get_host_props,
	.enable_interrupt = csk6_sdhc_enable_interrupt,
	.disable_interrupt = csk6_sdhc_disable_interrupt,
};

#ifdef CONFIG_NOCACHE_MEMORY
#define CSK6_SDHC_NOCACHE_TAG __attribute__((__section__(".nocache")));
#else
#define CSK6_SDHC_NOCACHE_TAG
#endif

#ifdef CONFIG_CSK6_SDHC_DMA_SUPPORT
#define CSK6_SDHC_DMA_BUFFER_DEFINE(n)                                                             \
	static uint32_t __aligned(32)                                                              \
	sdhc_##n##_dma_descriptor[CONFIG_CSK6_SDHC_DMA_BUFFER_SIZE / 4] CSK6_SDHC_NOCACHE_TAG;
#define CSK6_SDHC_DMA_BUFFER_INIT(n)                                                               \
	.sdhc_dma_descriptor = sdhc_##n##_dma_descriptor,                                          \
	.dma_descriptor_len = CONFIG_CSK6_SDHC_DMA_BUFFER_SIZE / 4,
#else
#define CSK6_SDHC_DMA_BUFFER_DEFINE(n)
#define CSK6_SDHC_DMA_BUFFER_INIT(n)
#endif /* CONFIG_CSK6_SDHC_DMA_SUPPORT */

#define CSK6_SDHC_INIT(n)                                                                          \
	static void sdhc_##n##_irq_config_func(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), csk6_sdhc_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct csk6_sdhc_config sdhc_##n##_config = {                                 \
		.base = (ftsdc021_reg *)DT_INST_REG_ADDR(n),                                       \
		.pwr_gpio = GPIO_DT_SPEC_INST_GET_OR(n, pwr_gpios, {0}),                           \
		.detect_gpio = GPIO_DT_SPEC_INST_GET_OR(n, cd_gpios, {0}),                         \
		.irq_config_func = sdhc_##n##_irq_config_func,                                     \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
	};                                                                                         \
                                                                                                   \
	CSK6_SDHC_DMA_BUFFER_DEFINE(n)                                                             \
                                                                                                   \
	static struct csk6_sdhc_data sdhc_##n##_data = {.card_present = false,                     \
							CSK6_SDHC_DMA_BUFFER_INIT(n)};             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &csk6_sdhc_init, NULL, &sdhc_##n##_data, &sdhc_##n##_config,      \
			      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &sdhc_api);

DT_INST_FOREACH_STATUS_OKAY(CSK6_SDHC_INIT)

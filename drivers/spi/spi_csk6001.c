/*
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma GCC push_options
#pragma GCC optimize("O0")

#define DT_DRV_COMPAT listenai_csk_spi

#include <assert.h>
#include <kernel.h>
#include <errno.h>
#include <drivers/spi.h>
#include <drivers/clock_control.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(csk6001_spi, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#include "hal/spi.h"
#include "hal/Driver_SPI.h"
#include "hal/IOMuxManager.h"

static void spi_control_thread(void *, void *, void *);

K_QUEUE_DEFINE(spi_control_queue);
K_THREAD_DEFINE(spi_ctrl_thread_id, CONFIG_SPI_CTRL_STACK_SIZE,
                spi_control_thread, NULL, NULL, NULL,
                CONFIG_SPI_CTRL_PRIORITY, 0, 0);

typedef enum {
	SPI_OP_TRANSMIT,
	SPI_OP_RECEIVE,
	SPI_OP_TRANSCEIVE,
} spi_ctrl_opcode_t;

typedef struct {
	void *queue_reserved;
	spi_ctrl_opcode_t opcode;
	const struct device *dev;
	const void *tx_buf;
	void *rx_buf;
	uint32_t trans_len;
} spi_control_msg_t;

typedef void *spi_driver_ctx_t;

struct csk_spi_data {
	struct spi_context ctx;
	size_t current_transfer_len;
	spi_driver_ctx_t driver_context;
};

/* Fixme: remove this thread. The driver api could not call in isr context, so we need another thread now */
static void spi_control_thread(void *arg1, void *arg2, void *arg3)
{
	while (1) {
		spi_control_msg_t *msg = k_queue_get(&spi_control_queue, K_FOREVER);
		struct csk_spi_data *data = msg->dev->data;
		switch (msg->opcode) {
			case SPI_OP_TRANSMIT:
				SPI_Send(data->driver_context, msg->tx_buf, msg->trans_len);
			break;

			case SPI_OP_RECEIVE:
				SPI_Receive(data->driver_context, msg->rx_buf, msg->trans_len);
			break;

			case SPI_OP_TRANSCEIVE:
				SPI_Transfer(data->driver_context, msg->tx_buf, msg->rx_buf, msg->trans_len);
			break;
			default:
			break;
		}
	}
}

static void spi_ctrl_transceive(const struct device *dev, const void *tx_buf, void *rx_buf, uint32_t trans_len)
{
	spi_control_msg_t msg;
	msg.dev = dev;
	msg.trans_len = trans_len;
	msg.tx_buf = tx_buf;
	msg.rx_buf = rx_buf;
	if ((tx_buf != NULL) && (rx_buf != NULL)) {
		msg.opcode = SPI_OP_TRANSCEIVE;
	} else if (tx_buf != NULL) {
		msg.opcode = SPI_OP_TRANSMIT;
	} else if (rx_buf != NULL) {
		msg.opcode = SPI_OP_RECEIVE;
	} else {

	}
	k_queue_append(&spi_control_queue, &msg);
}

static int csk_spi_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct csk_spi_data *data = dev->data;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		return 0;
	}

	uint32_t spi_op_mode = (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_MASTER)
							? CSK_SPI_MODE_MASTER : CSK_SPI_MODE_SLAVE;

	uint32_t spi_sample_mode = 0;
	bool spi_cpol_mode = SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL;
	bool spi_cpha_mode = SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA;
	if (spi_cpol_mode) {  /* cpol = active low */
		if (spi_cpha_mode) {  /* cpha = second edge */
			spi_sample_mode = CSK_SPI_CPOL1_CPHA1;
		} else {  /* cpha = first edge */
			spi_sample_mode = CSK_SPI_CPOL1_CPHA0;
		}
	} else {  /* cpol = active high */
		if (spi_cpha_mode) {  /* cpha = second edge */
			spi_sample_mode = CSK_SPI_CPOL0_CPHA1;
		} else {  /* cpha = first edge */
			spi_sample_mode = CSK_SPI_CPOL0_CPHA0;
		}
	}
	uint32_t spi_word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	uint32_t spi_word_size_mask = CSK_SPI_DATA_BITS(spi_word_size);
	uint32_t spi_bits_order = (spi_cfg->operation & SPI_TRANSFER_LSB) ?
								CSK_SPI_LSB_MSB : CSK_SPI_MSB_LSB;

	uint32_t spi_dma_en_mask = 0;

#if	!CONFIG_SPI_TX_DMA
	spi_dma_en_mask |= CSK_SPI_TXIO_PIO;
#endif

#if	!CONFIG_SPI_RX_DMA
	spi_dma_en_mask |= CSK_SPI_RXIO_PIO;
#endif

	SPI_Control(data->driver_context, spi_op_mode | spi_sample_mode | spi_word_size_mask |
                spi_bits_order | spi_dma_en_mask, spi_cfg->frequency);

	data->ctx.config = spi_cfg;
    spi_context_cs_configure(&data->ctx);
    return 0;
}

static void csk_spi_hal_transceive(const struct device *dev)
{
	struct csk_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t chunk_len = spi_context_max_continuous_chunk(ctx);

	if (chunk_len == 0) {
        spi_context_cs_control(&data->ctx, false);
		spi_context_complete(ctx, 0);
		return;
	}

	if (ctx->tx_buf != NULL && ctx->rx_buf != NULL) {
		spi_ctrl_transceive(dev, ctx->tx_buf, ctx->rx_buf, chunk_len);
	} else if (ctx->tx_buf != NULL) {
		spi_ctrl_transceive(dev, ctx->tx_buf, NULL, chunk_len);
	} else if (ctx->rx_buf != NULL) {
		spi_ctrl_transceive(dev, NULL, ctx->rx_buf, chunk_len);
	} else {
        /* should not go into here */
	}
	data->current_transfer_len = chunk_len;
}

static int transceive(const struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      struct k_poll_signal *signal)
{
	struct csk_spi_data *data = dev->data;

	spi_context_lock(&data->ctx, asynchronous, signal, spi_cfg);
	int ret = csk_spi_configure(dev, spi_cfg);
	if (ret) {
		goto out;
	}
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);
    spi_context_cs_control(&data->ctx, true);
	csk_spi_hal_transceive(dev);
	ret = spi_context_wait_for_completion(&data->ctx);

out:
	spi_context_release(&data->ctx, ret);
	return ret;
}

static int csk_spi_transceive(const struct device *dev,
			       const struct spi_config *spi_cfg,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int csk_spi_transceive_async(const struct device *dev,
				     const struct spi_config *spi_cfg,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     struct k_poll_signal *async)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, async);
}
#endif /* CONFIG_SPI_ASYNC */

static int csk_spi_release(const struct device *dev, const struct spi_config *spi_cfg)
{
    struct csk_spi_data *data = dev->data;
    spi_context_unlock_unconditionally(&data->ctx);
    return 0;
}

static void csk_spi_isr_callback(uint32_t event, uint32_t usr_param)
{
	const struct device *dev = (const struct device *)usr_param;
	struct csk_spi_data *data = dev->data;
	if (event & CSK_SPI_EVENT_TRANSFER_COMPLETE) {
		spi_context_update_tx(&data->ctx, 1, data->current_transfer_len);
		spi_context_update_rx(&data->ctx, 1, data->current_transfer_len);
		csk_spi_hal_transceive(dev);
	}
    /* TODO: add transfer error processing */
}

static void csk_zephyr_spi_isr(const struct device *dev)
{
	extern void spi_irq_handler(SPI_DEV *spi);
	struct csk_spi_data *data = dev->data;
	spi_irq_handler(data->driver_context); /* Go into spi hal driver isr handler */
}

static const struct spi_driver_api csk_spi_driver_api = {
	.transceive = csk_spi_transceive,
 #ifdef CONFIG_SPI_ASYNC
 	.transceive_async = csk_spi_transceive_async,
 #endif
	 .release = csk_spi_release,
};

#define SPI_CONFIG_CS(n, func) \
do {                     \
    const char *cs_gpio_label = DT_LABEL(DT_GPIO_CTLR_BY_IDX(DT_NODELABEL(spi##n), cs_gpios, 0)); \
    const int cs_gpio_label_len = strlen(cs_gpio_label);                                        \
    int cs_pad_num = (cs_gpio_label[cs_gpio_label_len - 1] == 'A')? 0 :                         \
                        (cs_gpio_label[cs_gpio_label_len - 1] == 'B')? 1 : -1;                  \
    assert(cs_pad_num != -1);  \
	ARG_UNUSED(cs_pad_num);	\
    IOMuxManager_PinConfigure(sclk_pad_num, DT_PHA_BY_IDX(DT_NODELABEL(spi##n), cs_gpios, 0, pin), func);\
} while (0)

#define SPI_CONFIG_PINS(n, func)	do { \
	_Static_assert(DT_NODE_HAS_PROP(DT_NODELABEL(spi##n), mosi_gpios), "Could not found mosi-gpio"); \
	_Static_assert(DT_NODE_HAS_PROP(DT_NODELABEL(spi##n), miso_gpios), "Could not found miso-gpio"); \
	_Static_assert(DT_NODE_HAS_PROP(DT_NODELABEL(spi##n), sclk_gpios), "Could not found sclk-gpio"); \
                                \
     /* spi signals will always locate at IOMUX function 6(spi0) and 7(spi1) */                   \
    const char *mosi_gpio_label = DT_LABEL(DT_GPIO_CTLR_BY_IDX(DT_NODELABEL(spi##n), mosi_gpios, 0));\
    const int mosi_gpio_label_len = strlen(mosi_gpio_label);                                      \
    int mosi_pad_num = (mosi_gpio_label[mosi_gpio_label_len - 1] == 'A')? 0 :                     \
                        (mosi_gpio_label[mosi_gpio_label_len - 1] == 'B')? 1 : -1;                \
                                 \
    const char *miso_gpio_label = DT_LABEL(DT_GPIO_CTLR_BY_IDX(DT_NODELABEL(spi##n), miso_gpios, 0));\
    const int miso_gpio_label_len = strlen(miso_gpio_label);                                      \
    int miso_pad_num = (miso_gpio_label[miso_gpio_label_len - 1] == 'A')? 0 :                     \
                        (miso_gpio_label[miso_gpio_label_len - 1] == 'B')? 1 : -1;                \
                                 \
    const char *sclk_gpio_label = DT_LABEL(DT_GPIO_CTLR_BY_IDX(DT_NODELABEL(spi##n), sclk_gpios, 0));\
    const int sclk_gpio_label_len = strlen(sclk_gpio_label);                                      \
    int sclk_pad_num = (sclk_gpio_label[sclk_gpio_label_len - 1] == 'A')? 0 :                     \
                        (sclk_gpio_label[sclk_gpio_label_len - 1] == 'B')? 1 : -1;                \
    assert(mosi_pad_num != -1 && mosi_pad_num != -1 && sclk_pad_num != -1);                       \
    IOMuxManager_PinConfigure(mosi_pad_num, DT_PHA_BY_IDX(DT_NODELABEL(spi##n), mosi_gpios, 0, pin), func);	\
    IOMuxManager_PinConfigure(miso_pad_num, DT_PHA_BY_IDX(DT_NODELABEL(spi##n), miso_gpios, 0, pin), func);	\
    IOMuxManager_PinConfigure(sclk_pad_num, DT_PHA_BY_IDX(DT_NODELABEL(spi##n), sclk_gpios, 0, pin), func); \
    SPI##n##_CONFIG_CS(n);          \
} while(0)

#define SPI_CSK_SPI_INIT(n)						             \
	static int csk_spi_##n##_init(const struct device *dev); \
	static struct csk_spi_data csk_spi_##n##_data = {	     \
		SPI_CONTEXT_INIT_LOCK(csk_spi_##n##_data, ctx),		 \
		SPI_CONTEXT_INIT_SYNC(csk_spi_##n##_data, ctx),		 \
	};	\
	DEVICE_DT_DEFINE(DT_NODELABEL(spi##n),                                 \
				&csk_spi_##n##_init,                         \
				NULL,			                             \
			    &csk_spi_##n##_data,				         \
			    NULL,		                                 \
				POST_KERNEL,				                 \
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		     \
			    &csk_spi_driver_api);			             \
        \
	static int csk_spi_##n##_init(const struct device *dev)	 \
	{	\
		struct csk_spi_data *data = dev->data;		         \
		data->driver_context = SPI##n();		             \
		/* Fixme: rework the SPI_Initialize(), let the user parameter become void* type */  \
        SPI_Initialize(data->driver_context, csk_spi_isr_callback, (uint32_t)dev);                      \
		SPI_PowerControl(data->driver_context, CSK_POWER_FULL);                                         \
		IRQ_CONNECT(DT_IRQN(DT_NODELABEL(spi##n)), DT_IRQ(DT_NODELABEL(spi##n), priority),	            \
			        csk_zephyr_spi_isr, DEVICE_DT_GET(DT_NODELABEL(spi##n)), 0);       \
		irq_enable(DT_IRQN(DT_NODELABEL(spi##n)));  \
        SPI##n##_CONFIG_PINS(n);                                      \
		spi_context_unlock_unconditionally(&data->ctx);		 \
		return 0;						                     \
	}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi0), okay)
#if DT_NODE_HAS_PROP(DT_NODELABEL(spi0), cs_gpios)
    #define SPI0_CONFIG_CS(n)   SPI_CONFIG_CS(n, CSK_IOMUX_FUNC_ALTER6)
#else
    #define SPI0_CONFIG_CS(n)
#endif
#define SPI0_CONFIG_PINS(n)  SPI_CONFIG_PINS(n, CSK_IOMUX_FUNC_ALTER6)
SPI_CSK_SPI_INIT(0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay)
#if DT_NODE_HAS_PROP(DT_NODELABEL(spi1), cs_gpios)
    #define SPI1_CONFIG_CS(n)   SPI_CONFIG_CS(n, CSK_IOMUX_FUNC_ALTER7)
#else
    #define SPI1_CONFIG_CS(n)
#endif
#define SPI1_CONFIG_PINS(n)  SPI_CONFIG_PINS(n, CSK_IOMUX_FUNC_ALTER7)
SPI_CSK_SPI_INIT(1);
#endif

#pragma GCC pop_options

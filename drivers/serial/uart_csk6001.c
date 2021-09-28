/*
 * Copyright (c) 2018 Diego Sueiro <diego.sueiro@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT listenai_csk_uart

/**
 * @brief Driver for UART on NXP IMX family processor.
 *
 * For full serial function, use the USART controller.
 *
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <sys/__assert.h>
#include <soc.h>
#include <init.h>
#include <drivers/uart.h>

#include "hal/Driver_UART.h"
#include "hal/uart.h"

typedef void *uart_driver_ctx_t;

struct csk_uart_config {
    int driver_num;
    void *arg;
};

static const uart_driver_ctx_t csk_uart_get_driver_ctx(int port)
{
    if (port == 0) {
        return UART0();
    } else if (port == 1) {
        return UART1();
    } else if (port == 2) {
        return UART2();
    }
    return NULL;
}

static int csk_uart_poll_in(const struct device *dev, unsigned char *p_char)
{
    return 0;
}

static void csk_uart_poll_out(const struct device *dev, unsigned char out_char)
{
    const struct csk_uart_config *config = dev->config;
    uart_driver_ctx_t driver_context = csk_uart_get_driver_ctx(config->driver_num);
    DW_UART_RegDef *uart_reg = ((UART_RESOURCES*)driver_context)->reg;
    uart_reg->REG_THR.all = out_char;
    while(!uart_reg->REG_LSR.bit.temt);
}

static void csk_uart_isr(uint32_t event, void* arg)
{

}

static int csk_uart_init(const struct device *dev)
{
    uart_driver_ctx_t uart_driver_ctx = csk_uart_get_driver_ctx(0);  //only support for uart0
    UART_Initialize(uart_driver_ctx, csk_uart_isr, (void *) uart_driver_ctx);
    UART_PowerControl(uart_driver_ctx, CSK_POWER_FULL);
    UART_Control(uart_driver_ctx, CSK_UART_MODE_ASYNCHRONOUS |
                                    CSK_UART_DATA_BITS_8 |
                                    CSK_UART_PARITY_NONE |
                                    CSK_UART_STOP_BITS_1 |
                                    CSK_UART_FLOW_CONTROL_NONE |
                                    CSK_UART_GPIO_CONTROL_DEFAULT, 115200);
    UART_Control(uart_driver_ctx, CSK_UART_CONTROL_TX, 1);
    UART_Control(uart_driver_ctx, CSK_UART_CONTROL_RX, 1);
    IOMuxManager_PinConfigure(CSK_IOMUX_PAD_A, 3, CSK_IOMUX_FUNC_ALTER2);
    return 0;
}


static const struct uart_driver_api csk_uart_driver_api = {
	.poll_in  = csk_uart_poll_in,
	.poll_out = csk_uart_poll_out,
};


#define UART_CSK_INIT(n)						\
    static const struct csk_uart_config csk_uart_##n##_config = {	\
		.driver_num = n,                           \
	};  \
									\
	DEVICE_DT_INST_DEFINE(n, \
            &csk_uart_init, \
            NULL,			\
			NULL,   \
            &csk_uart_##n##_config,	\
			PRE_KERNEL_1,					\
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			&csk_uart_driver_api);				\

DT_INST_FOREACH_STATUS_OKAY(UART_CSK_INIT)


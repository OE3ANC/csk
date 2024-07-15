/*
 * Copyright (c) 2023 Anhui(Shenzhen) Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <string.h>

#define BUF_SIZE 9


uint8_t *master_buffer_tx = NULL;
uint8_t *master_buffer_rx = NULL;


#define MASTER_STACK_SIZE 4096

K_THREAD_STACK_DEFINE(master_stack_area, MASTER_STACK_SIZE);


struct k_thread master_thread_data;

const struct device *spi_master = NULL;

int spi_master_dev_init(void)
{
	spi_master = DEVICE_DT_GET(DT_NODELABEL(spi0));
	if (spi_master == NULL) {
		printk("spi_master: Device is not found.\n");
		return -ENODEV;
	}

	if (!device_is_ready(spi_master)) {
		printk("spi_master: Device is not ready.\n");
		return -ENODEV;
	}
    master_buffer_tx = k_aligned_alloc(32,BUF_SIZE);
    for(int i=0;i<BUF_SIZE;i++){
        master_buffer_tx[i] = i;
    }

    master_buffer_rx = k_aligned_alloc(32,BUF_SIZE);
	return 0;
}


void master_thread(void *v1, void *v2, void *v3)
{
	struct spi_config spi_cfg = { 0 };


	const struct spi_buf tx_bufs[] = {
		{
			.buf = master_buffer_tx,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = master_buffer_rx,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf_set tx = { .buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs) };
	const struct spi_buf_set rx = { .buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs) };

	/* spi master 8bit, LSB first*/
	spi_cfg.operation = SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_TRANSFER_LSB;
	spi_cfg.frequency = 10 * 1000000UL;
    printk("[Master][%s %d]\n", __FUNCTION__, __LINE__);
	while (1) {
		k_msleep(1000);
		memset(master_buffer_rx, 0, BUF_SIZE);
        printk("[Master][%s %d]\n", __FUNCTION__, __LINE__);
		spi_transceive(spi_master, &spi_cfg, &tx, &rx);
		printk("[Master]receive:");
		for (uint8_t i = 0; i < BUF_SIZE; i++) {
			printk("0x%02x ", master_buffer_rx[i]);
		}
		printk("\n");
	}
}


void spi_master_init(void)
{
	int ret = 0;

	printk("SPI test start \n");

	ret = spi_master_dev_init();
	if(ret != 0){
		return;
	}

	/* create master thread */
	int pri = k_thread_priority_get(k_current_get());

	k_thread_create(&master_thread_data, master_stack_area,
			K_THREAD_STACK_SIZEOF(master_stack_area), master_thread, NULL, NULL, NULL,
			pri, 0, K_NO_WAIT);

	printk("SPI master thread created\n");

}

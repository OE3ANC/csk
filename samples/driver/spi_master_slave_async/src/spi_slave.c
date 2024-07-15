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

uint8_t *slave_buffer_tx = NULL;
uint8_t *slave_buffer_rx = NULL;



#define SLAVE_STACK_SIZE 4096

K_THREAD_STACK_DEFINE(slave_stack_area, SLAVE_STACK_SIZE);

struct k_thread slave_thread_data;
const struct device *spi_slave = NULL;

static struct k_poll_signal async_sig = K_POLL_SIGNAL_INITIALIZER(async_sig);
static struct k_poll_event async_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
				 K_POLL_MODE_NOTIFY_ONLY,
				 &async_sig);
static K_SEM_DEFINE(caller_sem, 0, 1);


int spi_slave_dev_init(void)
{
	spi_slave = DEVICE_DT_GET(DT_NODELABEL(spi1));
	if (spi_slave == NULL) {
		printk("spi_slave: Device is not found.\n");
		return -ENODEV;
	}

	if (!device_is_ready(spi_slave)) {
		printk("spi_slave: Device is not ready.\n");
		return -ENODEV;
	}
    slave_buffer_tx = k_aligned_alloc(32,BUF_SIZE);
    for(int i=0;i<BUF_SIZE;i++){
        slave_buffer_tx[i] = i;
    }

    slave_buffer_rx = k_aligned_alloc(32,BUF_SIZE);
	return 0;
}

static void spi_async_call_cb(void* ptr1,void *ptr2,void *ptr3)
{
	int ret;

	printk("Polling...");

	while (1) {
		ret = k_poll(&async_evt, 1, K_FOREVER);
        if(ret){
            printk("one or more events are not ready,ret:%d\n",ret);
            continue;
        }

        printk("Got data");

	
		k_sem_give(&caller_sem);

		/* Reinitializing for next call */
		async_evt.signal->signaled = 0U;
		async_evt.state = K_POLL_STATE_NOT_READY;
	}
}


void slave_thread(void *v1, void *v2, void *v3)
{
	struct spi_config spi_cfg = { 0 };

	const struct spi_buf tx_bufs[] = {
		{
			.buf = slave_buffer_tx,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = slave_buffer_rx,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf_set tx = { .buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs) };
	const struct spi_buf_set rx = { .buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs) };

	/* spi slave 8bit, LSB first*/
	spi_cfg.operation = SPI_WORD_SET(8) | SPI_OP_MODE_SLAVE | SPI_TRANSFER_LSB;
	spi_cfg.frequency = 10 * 1000000UL;

	while (1) {
		memset(slave_buffer_rx, 0, BUF_SIZE);
		// spi_transceive(spi_slave, &spi_cfg, &tx, &rx);
		spi_transceive_signal(spi_slave,&spi_cfg, &tx, &rx, &async_sig);

        k_sem_take(&caller_sem, K_FOREVER);

		printk("[Slave]receive:");
		for (uint8_t i = 0; i < BUF_SIZE; i++) {
			printk("0x%02x ", slave_buffer_rx[i]);
		}
		printk("\n");
	}
}


void spi_slave_init(void)
{
	int ret = 0;

	printk("SPI test start \n");
	ret = spi_slave_dev_init();
	if(ret != 0){
		return;
	}

	/* create slave thread */
	int pri = k_thread_priority_get(k_current_get());

	k_thread_create(&slave_thread_data, slave_stack_area,
			K_THREAD_STACK_SIZEOF(slave_stack_area), slave_thread, NULL, NULL, NULL,
			pri, 0, K_NO_WAIT);
	printk("SPI slave thread created\n");

	while (1) {
		k_msleep(10);
	}
}

K_THREAD_DEFINE(spi_async_thread_id, 4096, spi_async_call_cb, NULL, NULL, NULL, 2, 0, 1000);

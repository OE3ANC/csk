/*
 * Copyright (c) 2023 Anhui(Shenzhen) Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample, LOG_LEVEL_DBG);

#define NUM_SECTORS (1U)
static const char *disk_pdrv = CONFIG_SDMMC_VOLUME_NAME;
static uint32_t disk_sector_count;
static uint32_t disk_sector_size;
static uint8_t *wbuf, *rbuf;

int main(void)
{
	int ret = 0;
	uint32_t cmd_buf;

	ret = disk_access_init(disk_pdrv);

	ret = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, &cmd_buf);
	if (ret != 0) {
		LOG_ERR("Disk ioctl get sector count failed (%d)\n", ret);
		return -1;
	}

	disk_sector_count = cmd_buf;
	LOG_DBG("Disk reports %u sectors\n", cmd_buf);

	ret = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &cmd_buf);
	if (ret != 0) {
		LOG_ERR("Disk ioctl get sector size failed (%d)\n", ret);
		return -1;
	}

	disk_sector_size = cmd_buf;
	LOG_DBG("Disk reports sector size %u\n", cmd_buf);

	wbuf = (uint8_t *)k_malloc(NUM_SECTORS * disk_sector_size);
	if (wbuf == NULL) {
		LOG_ERR(" wbuf malloc failed\n");
	}

	for (uint32_t i = 0; i < NUM_SECTORS * disk_sector_size; i++) {
		wbuf[i] = i;
	}

	LOG_HEXDUMP_INF(wbuf, 256, "wbuf");

	ret = disk_access_write(disk_pdrv, wbuf, 1, NUM_SECTORS);
	if (ret) {
		LOG_ERR("disk_access_write failed (%d)\n", ret);
		return -1;
	}

	rbuf = (uint8_t *)k_malloc(NUM_SECTORS * disk_sector_size);
	if (rbuf == NULL) {
		LOG_ERR(" rbuf malloc failed\n");
	}
	memset(rbuf, 0, NUM_SECTORS * disk_sector_size);

	ret = disk_access_read(disk_pdrv, rbuf, 1, NUM_SECTORS);
	if (ret) {
		LOG_ERR("disk_access_write failed (%d)\n", ret);
		return -1;
	}

	LOG_HEXDUMP_INF(rbuf, 256, "rbuf");

	for (uint32_t i = 0; i < NUM_SECTORS * disk_sector_size; i++) {
		if (rbuf[i] != wbuf[i]) {
			LOG_ERR("Read data did not match data written to disk %d\n", i);
			return -1;
		}
	}

	LOG_DBG("sdcard sample done \n");

	return 0;
}

#if CONFIG_BOARD_CSK6_DUOMOTAI_DEVKIT

#include <zephyr/drivers/gpio.h>

#define SD_SWITCH_NODE DT_ALIAS(sd_switch)
static const struct gpio_dt_spec sd_switch = GPIO_DT_SPEC_GET(SD_SWITCH_NODE, gpios);

/*
    电路设计：需要将该引脚配置为低电平，SD引脚才能正常使用
*/
static int sd_switch_enable(void)
{
    gpio_pin_configure_dt(&sd_switch, GPIO_OUTPUT_ACTIVE);

    gpio_pin_set_dt(&sd_switch, 0);

    return 0;
}

/*
	选择SD引脚复用，需要在CH32驱动初始化之后，同时又要保障在disk驱动初始化之前
*/
SYS_INIT(sd_switch_enable, POST_KERNEL, 89);
#endif

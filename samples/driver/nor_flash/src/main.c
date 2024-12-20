/*
 * Copyright (c) 2023 Anhui(Shenzhen) Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include "drivers/csk6_driver.h"
#include <zephyr/storage/flash_map.h>

#if DT_NODE_HAS_STATUS(DT_INST(0, listenai_csk_flash_controller), okay)
#define FLASH_DEVICE DT_LABEL()
#define FLASH_NAME   "LISTENAI INN-FLASH"
#else
#error Unsupported flash driver
#endif

#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE
#define FLASH_TEST_OFFSET FIXED_PARTITION_OFFSET(image_1_nonsecure)
#else
#define FLASH_TEST_OFFSET FIXED_PARTITION_OFFSET(storage_partition)
#endif

#define FLASH_PAGE_SIZE	 4096


#define FLASH_SIZE_1K        (1 * 1024)
#define FLASH_SIZE_5K        (5 * 1024)
#define FLASH_SIZE_10K       (10 * 1024)
#define FLASH_SIZE_100K      (100 * 1024)
#define FLASH_SIZE_500K      (500 * 1024)
#define FLASH_SIZE_1M        (1 * 1024 * 1024)
#define FLASH_SIZE_2M        (2 * 1024 * 1024)
#define FLASH_SIZE_5M        (5 * 1024 * 1024)
#define FLASH_SIZE_TEST			    FLASH_SIZE_5M   
int main(void)
{
	const struct device *flash_dev;
	uint8_t write_data = 0x55; // 要写入的数据 0x55
	uint32_t offset;
	int ret;

	printf("\nLitenai CSK6 Flash Testing\n");
	printf("=========================\n");
	GPIO_SetDir(GPIOA(), (1 << 7), 1);
	k_msleep(1000);
	GPIO_PinWrite(GPIOA(), (1 << 7), 0);
	k_msleep(500);
	GPIO_PinWrite(GPIOA(), (1 << 7), 1);
	flash_dev = DEVICE_DT_GET(DT_INST(0, listenai_csk_flash_controller));

	if (!device_is_ready(flash_dev)) {
		printf("Litenai CSK6 flash driver was not found!\n");
		return -1;
	}

	if (flash_erase(flash_dev, FLASH_TEST_OFFSET, FLASH_SIZE_TEST) != 0) {
		printf("   Flash erase failed!\n");
	} else {
		printf("   Flash erase succeeded!\n");
	}
	GPIO_PinWrite(GPIOA(), (1 << 7), 0);
	k_msleep(500);
	GPIO_PinWrite(GPIOA(), (1 << 7), 1);
	printf("\nTest: Flash write 0x55 to the 2MB region starting at offset 0x%x\n",
	       FLASH_TEST_OFFSET);
	for (offset = FLASH_TEST_OFFSET; offset < FLASH_TEST_OFFSET + FLASH_SIZE_TEST;
	     offset += FLASH_PAGE_SIZE) {
		ret = flash_write(flash_dev, offset, &write_data, sizeof(write_data));
		if (ret != 0) {
			printf("   Flash write failed at offset 0x%x!\n", offset);
			return -1;
		}
	}

	printf("   Flash write succeeded!\n");
	GPIO_PinWrite(GPIOA(), (1 << 7), 0);
	k_msleep(500);
	GPIO_PinWrite(GPIOA(), (1 << 7), 1);
	return 0;
}

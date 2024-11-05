/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/device.h>
#include <zephyr/timing/timing.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include "zephyr/init.h"

LOG_MODULE_REGISTER(sample, LOG_LEVEL_DBG);

/* 最大支持block size为512字节 */
#define SECTOR_SIZE 512

#define SD_SWITCH_NODE DT_ALIAS(sd_switch)
static const struct gpio_dt_spec sd_switch = GPIO_DT_SPEC_GET(SD_SWITCH_NODE, gpios);

#define TEST_CNT	(10U)
static const char *disk_pdrv = CONFIG_SDMMC_VOLUME_NAME;
static uint32_t disk_sector_count;
static uint32_t disk_sector_size;

static uint8_t test_buf[200 * 1024] __aligned(32);

uint32_t bytes_per_tests[5] = {1024, 1024 *5, 1024 * 25, 1024 * 50, 1024 * 200};


int calculate_access_rate_KPS(uint32_t us, uint32_t bytes, uint32_t iterations_cnt)
{
	int rate = 0;
	rate = bytes * iterations_cnt * 1000 / us;
	return rate;
}

int disk_sdmmc_tests(uint32_t bytes)
{
	int ret = 0;
	timing_t start_time, end_time;
	uint64_t cycles;
	uint64_t write_time_ns,read_time_ns;
	uint32_t num_sector = bytes/disk_sector_size;

	for (uint32_t i = 0; i < num_sector * disk_sector_size; i++) {
		test_buf[i] = i % num_sector;
	}

	/* Start the timing system */
	timing_init();
	timing_start();

	start_time = timing_counter_get();
	for(uint32_t j = 0; j < TEST_CNT; j++){
		ret = disk_access_write(disk_pdrv, test_buf, 10, num_sector);
		if (ret) {
			LOG_ERR("disk_access_write failed (%d)\n", ret);
			ret = -EIO;
			goto error;
		}
	}
	end_time = timing_counter_get();
	cycles = timing_cycles_get(&start_time, &end_time);
	write_time_ns = timing_cycles_to_ns(cycles);
	timing_stop();


	/* Start the timing system */
	timing_init();
	timing_start();

	start_time = timing_counter_get();
	for(uint32_t j = 0; j < TEST_CNT; j++){
		ret = disk_access_read(disk_pdrv, test_buf, 10, num_sector);
		if (ret) {
			LOG_ERR("disk_access_write failed (%d)\n", ret);
			ret = -EIO;
			goto error;
		}
	}
	end_time = timing_counter_get();
	cycles = timing_cycles_get(&start_time, &end_time);
	read_time_ns = timing_cycles_to_ns(cycles);
	timing_stop();


	LOG_DBG(" %d bytes --- %llu us | %llu us\n", bytes, write_time_ns / (TEST_CNT * NSEC_PER_USEC), read_time_ns / (TEST_CNT * NSEC_PER_USEC));
	LOG_DBG(" %d bytes --- %d K/s | %d K/s\n", bytes, calculate_access_rate_KPS(write_time_ns / NSEC_PER_USEC, bytes, TEST_CNT), 
		calculate_access_rate_KPS(read_time_ns / NSEC_PER_USEC, bytes, TEST_CNT));
	return ret;
error:

	return ret;
}

void *test_csk_sdcard_setup(void)
{
	int rc;
	uint32_t cmd_buf;

	rc = disk_access_init(disk_pdrv);
	zassert_equal(rc, 0, "Disk access initialization failed");

	rc = disk_access_status(disk_pdrv);
	zassert_equal(rc, DISK_STATUS_OK, "Disk status is not OK");

	rc = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, &cmd_buf);
	zassert_equal(rc, 0, "Disk ioctl get sector count failed");

	TC_PRINT("Disk reports %u sectors\n", cmd_buf);
	disk_sector_count = cmd_buf;

	rc = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &cmd_buf);
	zassert_equal(rc, 0, "Disk ioctl get sector size failed");
	TC_PRINT("Disk reports sector size %u\n", cmd_buf);
	disk_sector_size = cmd_buf;

	zassert_true(cmd_buf <= SECTOR_SIZE,
		"Test will fail, SECTOR_SIZE definition must be increased");

	return NULL;
}

ZTEST(csk_sdcard_test, test_csk_sdcard_wr)
{
	int ret = 0;

	LOG_DBG("  datelen  ---   write | read \n");

	for(uint8_t i = 0; i < sizeof(bytes_per_tests)/sizeof(bytes_per_tests[0]); i++){
		ret = disk_sdmmc_tests(bytes_per_tests[i]);
		zassert_equal(ret, 0, "disk_sdmmc_tests failed, %d", ret);
	}
}

ZTEST_SUITE(csk_sdcard_test, NULL, test_csk_sdcard_setup, NULL, NULL, NULL);

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

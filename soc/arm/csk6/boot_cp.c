#include "venus_ap.h"

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/init.h>
#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(boot_cp);

#define MEM_BANK_CNT		4
#define SHAREMEMORY_BANK0_REMAP 1
#define AP_SYS_RAM_BANK_SIZE	0x00010000
#define AP_SYS_RAM_BANK_ADDR	(0x20080000 + AP_SYS_RAM_BANK_SIZE * MEM_BANK_CNT)

extern void dcache_invalidate_range(unsigned long start, unsigned long end);

#define PSRAM_START (0x60000000)
#define PSRAM_SIZE  (8 * 1024 * 1024)

#define PSRAM_START_ON_AP_CORE (PSRAM_START - 0x30000000)
#define PSRAM_START_ON_CP_CORE (PSRAM_START)

#define PSRAM_ADDR_CP_CONVERT_TO_AP(addr) ((uint32_t)(addr) - (uint32_t)0x30000000)
#define PSRAM_ADDR_AP_CONVERT_TO_CP(addr) ((uint32_t)(addr) + (uint32_t)0x30000000)

/* Only support boot cp from nor flash and psram */
void boot_cp(const void *run_address, const void *load_address, const uint32_t img_size)
{
	IP_SYSCTRL->REG_CP_CTRL2.bit.CP_BOOT_ADDR = (uint32_t)run_address;
	IP_SYSCTRL->REG_CP_CTRL1.bit.CP_STAT_VECTOR_SEL = 1;
	IP_SYSCTRL->REG_CP_CTRL1.bit.CP_HIFI4_RUNSTALL = 1;

	IP_SYSCTRL->REG_SW_RESET_CP.all = 0xCAFE100B;
	if (IP_SYSCTRL->REG_SW_RESET_NPU.all) {
		IP_SYSCTRL->REG_SW_RESET_NPU.all = 0xCAFE000C;
	}

	__DMB();
	__DSB();

	if (((uint32_t)run_address & 0x68000000) == 0x68000000) {
		(void)load_address;
		(void)img_size;
	} else if (((uint32_t)run_address & 0x60000000) == 0x60000000) {
		if (img_size == 0) {
			LOG_INF("The CP core does not start because the image size(0x%x) is "
				"invalid.",
				img_size);
		}
		/* cp core start from psram, need copy img to psram from load address */
		memcpy((void *)PSRAM_ADDR_CP_CONVERT_TO_AP(run_address), load_address, img_size);
		sys_cache_data_flush_range((void *)PSRAM_ADDR_CP_CONVERT_TO_AP(run_address), img_size);
		LOG_INF("Load CP image from address %p", load_address);
	} else {
		LOG_INF("The CP core does not start because the running address(%p) is invalid.",
			run_address);
		return;
	}

	LOG_INF("CP core starts at address 0x%p", run_address);

	IP_SYSCTRL->REG_SW_RESET_CP.all = 0xCAFE000B;
	IP_SYSCTRL->REG_SW_RESET_CP.all = 0xCAFE100B;
	IP_SYSCTRL->REG_CP_CTRL1.bit.CP_HIFI4_RUNSTALL = 0x0;

	IP_SYSCTRL->REG_AP_CTRL1.bit.AP_RAM_SEL = (SHAREMEMORY_BANK0_REMAP << MEM_BANK_CNT);
	memset((void *)AP_SYS_RAM_BANK_ADDR, 0x00, AP_SYS_RAM_BANK_SIZE);
	sys_cache_data_invd_range((void *)AP_SYS_RAM_BANK_ADDR,
				  AP_SYS_RAM_BANK_ADDR + AP_SYS_RAM_BANK_SIZE);

	IP_SYSCTRL->REG_MISC_CLK_CFG2.bit.ENA_USBC_CLK = 0x01;
}

#ifdef CONFIG_CSK6_BOOT_CP_CORE_ON_SYSINIT_APPLICATION
static int boot_cp_sys_init(void)
{
	boot_cp((void *)CONFIG_CSK6_BOOT_CP_CORE_IMAGE_RUN_ADDRESS,
		(void *)CONFIG_CSK6_BOOT_CP_CORE_IMAGE_LOAD_ADDRESS,
		CONFIG_CSK6_BOOT_CP_CORE_IMAGE_SIZE);

	return 0;
}

SYS_INIT(boot_cp_sys_init, APPLICATION, 0);
#endif

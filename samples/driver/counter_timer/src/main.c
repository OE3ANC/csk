/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>

#define DELAY_US 1000000
uint32_t time=0;
uint32_t start=0;
uint32_t g_counter = 0;
static void test_counter_interrupt_fn(const struct device *counter_dev, void *user_data)
{
	printk("g_counter: %d\n",g_counter);
	if(g_counter>=10){
		counter_stop(counter_dev);
		time =
		(double)(k_cycle_get_32() - start) / sys_clock_hw_cycles_per_sec() * 1000 * 1000;
		printk("cost time: %d us\n", time);
	}
	g_counter++;
}


int main(void)
{
	const struct device *counter_dev;
	int err;
	struct counter_top_cfg top_cfg;
	printk("Counter alarm sample\n\n");
#ifdef CONFIG_COUNTER_CSK6_GPTCHANNEL
	counter_dev = DEVICE_DT_GET(DT_NODELABEL(gptchannel0));
#elif
	counter_dev = DEVICE_DT_GET(DT_NODELABEL(dual_timer));
#endif
	if (counter_dev == NULL) {
		printk("counter device not found\n");
		return 0;
	}
	top_cfg.flags = 0;
	top_cfg.ticks = counter_us_to_ticks(counter_dev, DELAY_US);
	top_cfg.callback = test_counter_interrupt_fn;
	top_cfg.user_data = NULL;
	err = counter_set_top_value(counter_dev, &top_cfg);
	if (err < 0) {
		printk("counter_set_channel_alarm err %d\n",err);
	}
	while (1)
	{
		g_counter = 0;
		printk("start\n");
		counter_start(counter_dev);

		k_msleep(12000);
		printk("restart\n");
		start = k_cycle_get_32();
	}	
	return 0;
}

#include <lisa_time.h>
#include <lisa_log.h>

#define TAG "lisa_time"
uint32_t lisa_os_get_ticks(void)
{
	return sys_clock_tick_get_32();
	//return k_cycle_get_32();
}

uint32_t lisa_os_set_ticks(uint32_t tick)
{
	__ASSERT(0, "%s is not supportd\n", __FUNCTION__);
	return 0;
}

uint32_t lisa_os_get_time(void)
{
	return (k_uptime_get() / 1000);
}

uint32_t lisa_rand32(void)
{
	return (uint32_t)lisa_os_get_ticks();
}
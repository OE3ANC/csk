#include <lisa_mem.h>
#include <lisa_time.h>
#include <lisa_log.h>
#include <lisa_thread.h>
#define THREAD_TIMESLICE 5

#define TAG "lisa_thread"
typedef void (*lisa_thread_entry_t)(void *, void *, void *);

lisa_thread_t *lisa_thread_create(const lisa_thread_attr_t *attr, void (*entry)(void *), void *arg)
{
	lisa_thread_t *lisa_thread = NULL;

	lisa_thread = (lisa_thread_t *)lisa_mem_calloc(1, sizeof(lisa_thread_t));
	if (lisa_thread == NULL) {
		LISA_LOGE(TAG, "[%s %d]lisa_mem_calloc FAILED!", __FUNCTION__, __LINE__);
		return NULL;
	}

	lisa_thread->thread_id =
		k_thread_create(&lisa_thread->handle, (k_thread_stack_t *)attr->stack,
				attr->stack_size, (lisa_thread_entry_t)entry, arg, NULL, NULL,
				attr->priority, 0, K_NO_WAIT);
	k_thread_system_pool_assign(&lisa_thread->handle);
	return lisa_thread;
}

lisa_err_t lisa_thread_delete(lisa_thread_t *thread)
{
	int iret;
	if (thread == NULL) {
		LISA_LOGE(TAG, "[%s %d]Input paramter error!", __FUNCTION__, __LINE__);
		return LISA_FAIL;
	}

	k_thread_abort(thread->thread_id);
	if (true == k_is_in_isr()) {
		iret = k_thread_join(&thread->handle, K_NO_WAIT);
	} else {
		iret = k_thread_join(&thread->handle, K_FOREVER);
	}
	if (0 != iret) {
		LISA_LOGE(TAG, "[%s %d]k_thread_join failed,errno=%d!", __FUNCTION__, __LINE__,
			  iret);
		return LISA_FAIL;
	}

	lisa_mem_free(thread);

	return LISA_OK;
}

lisa_err_t lisa_thread_yield(void)
{
	k_yield();
	return LISA_OK;
}

lisa_err_t lisa_thread_suspend(lisa_thread_t *thread)
{
	if (thread == NULL) {
		LISA_LOGE(TAG, "[%s %d]Input paramter error!", __FUNCTION__, __LINE__);
		return LISA_FAIL;
	}
	k_thread_suspend(thread->thread_id);
	return LISA_OK;
}

lisa_err_t lisa_thread_resume(lisa_thread_t *thread)
{
	if (thread == NULL) {
		LISA_LOGE(TAG, "[%s %d]Input paramter error!", __FUNCTION__, __LINE__);
		return LISA_FAIL;
	}
	k_thread_resume(thread->thread_id);
	return LISA_OK;
}

lisa_err_t lisa_thread_delay(uint32_t ticks)
{
	k_timeout_t timout;
	timout.ticks = ticks;
	k_sleep(timout);
	return LISA_OK;
}

lisa_err_t lisa_thread_mdelay(uint32_t ms)
{
	k_msleep(ms);
	return LISA_OK;
}

lisa_thread_t *lisa_thread_getcurrent(void)
{
	// TODO
	__ASSERT(0, "%s is unsupport", __FUNCTION__);
	return NULL;
}
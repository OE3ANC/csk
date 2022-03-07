#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr.h>

#include <lisa_mem.h>
#include <lisa_log.h>

#define TAG "lisa_mem"

void *lisa_mem_alloc(uint32_t size)
{
	return k_malloc(size);
}

void lisa_mem_free(void *ptr)
{
	__ASSERT(ptr, "The ptr is NULL");
	k_free(ptr);
}

void *lisa_mem_calloc(uint32_t count, uint32_t size)
{
	void *p = NULL;
	p = k_malloc(count * size);
	if (NULL != p) {
		memset(p, 0, count * size);
	}
	return p;
}

void *lisa_mem_realloc(void *ptr, uint32_t size)
{
	__ASSERT(0, "%s is not supportd\n", __FUNCTION__);

	return NULL;
}

char *lisa_strdup(const char *s)
{
	__ASSERT(s, "The ptr is NULL");

	size_t len = strlen(s) + 1;
	char *tmp = (char *)lisa_mem_alloc(len);

	if (!tmp)
		return NULL;

	memcpy(tmp, s, len);

	return tmp;
}

void lisa_hexdump(char *buf, int len, int addr)
{
	__ASSERT(0, "%s is not supportd\n", __FUNCTION__);
}

void lisa_heap_info(void)
{
#if 0
	size_t curr_free = xPortGetFreeHeapSize();
	size_t histroy_min_free = xPortGetMinimumEverFreeHeapSize();

	printf("        current free heap        history min heap\n");
	printf("               %10d             %10d\n", curr_free, histroy_min_free);
	printf("\n");
	return;
#endif
}

uint32_t lisa_thread_info(void *task)
{
#if 0
	size_t curr_thread_stack = uxTaskGetStackHighWaterMark((TaskHandle_t)task);

	printf("  current thread stack:  ");
	printf("%20d\n", curr_thread_stack);
	printf("\n");
	return curr_thread_stack;
#else
	return 0;
#endif
}

//CSK_SHELL_CMD(lisa_heap_info, 系统栈信息);

// testself

//#define PSRAM_TESTSELF

#ifdef PSRAM_TESTSELF

#define PSRAM_SIZE_MAX (2 * 1024 * 1024)
static void psram_mem_rw_test(uint32_t start_address, uint32_t size)
{
	uint32_t i;
	uint8_t *p_uint8_t = NULL;

	// extern void rt_hw_wdg_stop(void);
	// rt_hw_wdg_stop();

	p_uint8_t = (uint8_t *)start_address;
	for (i = 0; i < size / sizeof(uint8_t); i++) {
		*p_uint8_t++ = (uint8_t)i;
	}

	p_uint8_t = (uint8_t *)start_address;
	for (i = 0; i < size / sizeof(uint8_t); i++) {
		if (*p_uint8_t != (uint8_t)i) {
			LISA_LOGE(TAG, "rd:%x,wd:%x ", (uint8_t)*p_uint8_t, (uint8_t)i);
			break;
		}
		p_uint8_t++;
	}

	// extern void rt_hw_wdg_start(int argc, char **argv);
	// rt_hw_wdg_start(0, NULL);
}

static void psram_mem_test(int argc, char *argv[])
{
	uint8_t *malloc_ptr = NULL;
	uint8_t *realloc_ptr = NULL;

	uint32_t nksize = atoi(argv[2]);

	LISA_LOGE(TAG, "nksize:0x%x", nksize);

	if (strcmp(argv[1], "malloc") == 0) {
		malloc_ptr = (uint8_t *)lisa_mem_alloc(nksize * 1024);
		if (!malloc_ptr) {
			LISA_LOGE(TAG, "lisa_mem_alloc fail");
		}

		LISA_LOGE(TAG, "malloc addr:0x%x", malloc_ptr);
		psram_mem_rw_test((uint32_t)malloc_ptr, nksize * 1024);

	} else if (strcmp(argv[1], "realloc") == 0) {
		uint8_t *p = (uint8_t *)lisa_mem_alloc(nksize * 1024);

		LISA_LOGE(TAG, "malloc addr:0x%x", p);
		realloc_ptr = lisa_mem_realloc(p, nksize * 1024 + 1024);
		if (!realloc_ptr) {
			LISA_LOGE(TAG, "lisa_mem_realloc fail");
		}

		LISA_LOGE(TAG, "remalloc addr:0x%x", realloc_ptr);
		psram_mem_rw_test((uint32_t)realloc_ptr, nksize * 1024 + 1024);

	} else if (strcmp(argv[1], "loop") == 0) {
		for (uint32_t i = 0; i < PSRAM_SIZE_MAX / 1024; i++) {
			// malloc_ptr = (uint8_t *)lisa_mem_alloc(i * 1024);
			malloc_ptr = (uint8_t *)lisa_mem_alloc(1024);
			if (!malloc_ptr) {
				LISA_LOGE(TAG, "lisa_mem_alloc fail %x", i);
				break;
			}
			LISA_LOGE(TAG, "malloc addr:0x%x", malloc_ptr);
			psram_mem_rw_test((uint32_t)malloc_ptr, i * 1024);

#if 0
			if (malloc_ptr) {
				LISA_LOGE(TAG, "free %p", malloc_ptr);
				lisa_mem_free(malloc_ptr);
				malloc_ptr = NULL;
			}
#endif
		}
	} else if (strcmp(argv[1], "dump") == 0) {
		malloc_ptr = (uint8_t *)lisa_mem_alloc(1 * 1024);
		if (!malloc_ptr) {
			LISA_LOGE(TAG, "lisa_mem_alloc fail");
		}

		LISA_LOGE(TAG, "malloc addr:0x%x", malloc_ptr);
		psram_mem_rw_test((uint32_t)malloc_ptr, 1 * 1024);
		lisa_hexdump((char *)malloc_ptr, 32, (int)malloc_ptr);
	}

	if (malloc_ptr) {
		LISA_LOGE(TAG, "free %p", malloc_ptr);
		lisa_mem_free(malloc_ptr);
		malloc_ptr = NULL;
	}

	if (realloc_ptr) {
		LISA_LOGE(TAG, "free %p", malloc_ptr);
		lisa_mem_free(realloc_ptr);
		realloc_ptr = NULL;
	}
}
CSK_SHELL_CMD(psram_mem_test, psram_mem_test);

#endif
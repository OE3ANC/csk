/*
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>
#include <string.h>
#include <zephyr/sys/math_extras.h>
#include "priv_heap.c"

void csk_free(void *ptr)
{
	struct k_heap **heap_ref;

	if (ptr != NULL)
	{
		heap_ref = ptr;
		ptr = --heap_ref;

		k_heap_free(*heap_ref, ptr);
	}
}

void *csk_malloc(size_t size)
{

	void *ret = priv_aligned_alloc(sizeof(void *), size);

	return ret;
}

void *csk_calloc(size_t nmemb, size_t size)
{
	void *ret;
	size_t bounds;

	if (size_mul_overflow(nmemb, size, &bounds))
	{

		return NULL;
	}

	ret = csk_malloc(bounds);
	if (ret != NULL)
	{
		(void)memset(ret, 0, bounds);
	}

	return ret;
}

void *csk_aligned_alloc(uint32_t align,size_t size)
{

	void *ret = priv_aligned_alloc(align, size);

	return ret;
}

void *csk_realloc(void *ptr, uint32_t bytes)
{
	void *ret = priv_aligned_realloc(ptr, sizeof(void *), bytes);

	return ret;
}

size_t csk_heap_free_size(void)
{
	size_t alloc_bytes, free_bytes = 0;
    struct sys_heap *heap = &(_PRIVATE_SYS_HEAP)->heap;
    extern void get_alloc_info(struct z_heap *h, size_t *alloc_bytes,
			   size_t *free_bytes);
	get_alloc_info(heap->heap, &alloc_bytes, &free_bytes);
    return free_bytes;
}

void csk_heap_info(void)
{
#if CONFIG_SYS_HEAP_INFO
	sys_heap_print_info(&(_PRIVATE_SYS_HEAP)->heap, 0);
#endif
}

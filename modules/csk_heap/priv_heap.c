Z_HEAP_DEFINE_IN_SECT(_private_sys_heap, CONFIG_CSK_HEAP_MEM_POOL_SIZE, __attribute__((section(CONFIG_CSK_HEAP_MEM_POOL_SECTION_NAME))));
#define _PRIVATE_SYS_HEAP (&_private_sys_heap)

static inline void *priv_heap_aligned_alloc(struct k_heap *heap, size_t align, size_t size)
{
	void *mem;
	struct k_heap **heap_ref;
	size_t __align;

	/*
	 * Adjust the size to make room for our heap reference.
	 * Merge a rewind bit with align value (see sys_heap_aligned_alloc()).
	 * This allows for storing the heap pointer right below the aligned
	 * boundary without wasting any memory.
	 */
	if (size_add_overflow(size, sizeof(heap_ref), &size))
	{
		return NULL;
	}
	__align = align | sizeof(heap_ref);

	mem = k_heap_aligned_alloc(heap, __align, size, K_NO_WAIT);
	if (mem == NULL)
	{
		return NULL;
	}

	heap_ref = mem;
	*heap_ref = heap;
	mem = ++heap_ref;
	__ASSERT(align == 0 || ((uintptr_t)mem & (align - 1)) == 0,
			 "misaligned memory at %p (align = %zu)", mem, align);

	return mem;
}

static inline void *priv_aligned_alloc(size_t align, size_t size)
{
	__ASSERT(align / sizeof(void *) >= 1 && (align % sizeof(void *)) == 0,
			 "align must be a multiple of sizeof(void *)");

	__ASSERT((align & (align - 1)) == 0,
			 "align must be a power of 2");

	void *ret = priv_heap_aligned_alloc(_PRIVATE_SYS_HEAP, align, size);

	return ret;
}

static inline void *__heap_aligned_realloc(struct k_heap *h, void *ptr, size_t align, size_t bytes,
										   k_timeout_t timeout)
{
	void *ret = NULL;
	k_spinlock_key_t key = k_spin_lock(&h->lock);

	ret = sys_heap_aligned_realloc(&h->heap, ptr, align, bytes);

	k_spin_unlock(&h->lock, key);
	return ret;
}

static inline void *priv_heap_aligned_realloc(struct k_heap *heap, void *ptr, size_t align, size_t size)
{
	void *mem;
	struct k_heap **heap_ref;
	size_t __align;

	/*
	 * Adjust the size to make room for our heap reference.
	 * Merge a rewind bit with align value (see sys_heap_aligned_alloc()).
	 * This allows for storing the heap pointer right below the aligned
	 * boundary without wasting any memory.
	 */
	if (size_add_overflow(size, sizeof(heap_ref), &size))
	{
		return NULL;
	}
	__align = align | sizeof(heap_ref);

	heap_ref = ptr;
	ptr = --heap_ref;

	mem = __heap_aligned_realloc(heap, ptr, __align, size, K_NO_WAIT);
	if (mem == NULL)
	{
		return NULL;
	}

	heap_ref = mem;
	*heap_ref = heap;
	mem = ++heap_ref;
	__ASSERT(align == 0 || ((uintptr_t)mem & (align - 1)) == 0,
			 "misaligned memory at %p (align = %zu)", mem, align);

	return mem;
}

static inline void *priv_aligned_realloc(void *ptr, size_t align, size_t size)
{
	__ASSERT(align / sizeof(void *) >= 1 && (align % sizeof(void *)) == 0,
			 "align must be a multiple of sizeof(void *)");

	__ASSERT((align & (align - 1)) == 0,
			 "align must be a power of 2");

	void *ret = priv_heap_aligned_realloc(_PRIVATE_SYS_HEAP, ptr, align, size);

	return ret;
}

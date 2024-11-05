#ifndef __CSK_MALLOC_H__
#define __CSK_MALLOC_H__

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Allocate memory from the heap.
     *
     * This routine provides traditional malloc() semantics. Memory is
     * allocated from the heap memory pool.
     *
     * @param size Amount of memory requested (in bytes).
     *
     * @return Address of the allocated memory if successful; otherwise NULL.
     */
    extern void *csk_malloc(size_t size);

    /**
     * @brief Free memory allocated from heap.
     *
     * This routine provides traditional csk_free() semantics. The memory being
     * returned must have been allocated from the heap memory pool or
     * csk_malloc().
     *
     * If @a ptr is NULL, no operation is performed.
     *
     * @param ptr Pointer to previously allocated memory.
     *
     * @return N/A
     */
    extern void csk_free(void *ptr);

    /**
     * @brief Allocate memory from heap, array style
     *
     * This routine provides traditional csk_calloc() semantics. Memory is
     * allocated from the heap memory pool and zeroed.
     *
     * @param nmemb Number of elements in the requested array
     * @param size Size of each array element (in bytes).
     *
     * @return Address of the allocated memory if successful; otherwise NULL.
     */
    extern void *csk_calloc(size_t nmemb, size_t size);

    /** @brief Expand the size of an existing allocation
     *
     * @note The return of a NULL on failure is a different behavior than
     * POSIX realloc(), which specifies that the original pointer will be
     * returned (i.e. it is not possible to safely detect realloc()
     * failure in POSIX, but it is here).
     *
     * @param ptr Original pointer returned from a previous allocation
     * @param bytes Number of bytes requested for the new block
     * @return Pointer to memory the caller can now use, or NULL
     */
    extern void *csk_realloc(void *ptr, uint32_t bytes);
    
    /**
     * @brief Allocate memory from the heap with a specified alignment.
     *
     * This routine provides semantics similar to aligned_alloc(); memory is
     * allocated from the heap with a specified alignment. However, one minor
     * difference is that csk_aligned_alloc() accepts any non-zero @p size,
     * whereas aligned_alloc() only accepts a @p size that is an integral
     * multiple of @p align.
     *
     * Above, aligned_alloc() refers to:
     * C11 standard (ISO/IEC 9899:2011): 7.22.3.1
     * The aligned_alloc function (p: 347-348)
     *
     * @param align Alignment of memory requested (in bytes).
     * @param size Amount of memory requested (in bytes).
     *
     * @return Address of the allocated memory if successful; otherwise NULL.
     */   
    extern void *csk_aligned_alloc(uint32_t align,size_t size);

    extern void csk_heap_info(void);

    void *csk_realloc(void *ptr, uint32_t bytes);

    size_t csk_heap_free_size(void);

#ifdef __cplusplus
}
#endif

#endif

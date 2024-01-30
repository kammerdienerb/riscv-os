#define _GNU_SOURCE

#include "hmalloc.h"
#include "internal.h"

#include "internal.c"
#include "internal_malloc.c"
#include "heap.c"
#include "thread.c"
#include "os.c"
#include "init.c"
#include "utils.h"

__attribute__((always_inline))
external inline void *hmalloc_malloc(size_t n_bytes) {
    return heap_alloc(get_this_thread_heap(), n_bytes);
}

external inline void * hmalloc_calloc(size_t count, size_t n_bytes) {
    void *addr;
    u64   new_n_bytes;

    new_n_bytes = count * n_bytes;
    addr        = hmalloc_malloc(new_n_bytes);

    memset(addr, 0, new_n_bytes);

    return addr;
}

external inline void * hmalloc_realloc(void *addr, size_t n_bytes) {
    void *new_addr;
    u64   old_size;

    new_addr = NULL;

    if (addr == NULL) {
        new_addr = hmalloc_malloc(n_bytes);
    } else {
        if (likely(n_bytes > 0)) {
            old_size = hmalloc_malloc_size(addr);
            /*
             * This is done for us in heap_alloc, but we'll
             * need the aligned value when we get the copy length.
             */
            n_bytes  = ALIGN(n_bytes, 8);

            /*
             * If it's already big enough, just leave it.
             * We won't worry about shrinking it.
             * Saves us an alloc, free, and memcpy.
             * Plus, we don't have to lock the thread.
             */
            if (old_size >= n_bytes) {
                return addr;
            }

            new_addr = hmalloc_malloc(n_bytes);
            memcpy(new_addr, addr, old_size);
        }

        hmalloc_free(addr);
    }

    return new_addr;
}

external inline void * hmalloc_reallocf(void *addr, size_t n_bytes) {
    return hmalloc_realloc(addr, n_bytes);
}

external inline void * hmalloc_valloc(size_t n_bytes) {
    heap_t *heap;
    void   *addr;

    heap = get_this_thread_heap();
    addr = heap_aligned_alloc(heap, n_bytes, system_info.page_size);

    return addr;
}

__attribute__((always_inline))
external inline void hmalloc_free(void *addr) {
    if (likely(addr != NULL)) {
        ASSERT(BLOCK_GET_HEAP_PTR(ADDR_PARENT_BLOCK(addr)) != NULL,
            "attempting to free from block that doesn't have a heap\n");

        heap_free(BLOCK_GET_HEAP_PTR(ADDR_PARENT_BLOCK(addr)), addr);
    }
}

external inline void * hmalloc_aligned_alloc(size_t alignment, size_t size) {
    heap_t *heap;
    void   *addr;

    heap = get_this_thread_heap();
    addr = heap_aligned_alloc(heap, size, alignment);

    return addr;
}

external inline size_t hmalloc_malloc_size(void *addr) {
    block_header_t *block;
    chunk_header_t *chunk;

    if (unlikely(addr == NULL)) { return 0; }

    block = ADDR_PARENT_BLOCK(addr);

    if (block->block_kind == BLOCK_KIND_CBLOCK) {
        chunk = CHUNK_FROM_USER_MEM(addr);

        if (unlikely(chunk->flags & CHUNK_IS_BIG)) {
            /*
             * Caculate size of cblock for big chunk.
             */
            return block->c.end - (void*)CHUNK_USER_MEM(chunk);
        }

        return CHUNK_SIZE(chunk);
    }
#ifdef HMALLOC_USE_SBLOCKS
    else if (likely(block->block_kind == BLOCK_KIND_SBLOCK)) {
        return block->s.size_class;
    }
#endif

    ASSERT(0, "couldn't determine size of allocation");

    return 0;
}

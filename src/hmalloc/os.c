#include "os.h"
#include "heap.h"
#include "init.h"
#include "internal.h"
#include "machine.h"
#include "page.h"

internal void system_info_init(void) {
    s64 page_size;

    page_size = PAGE_SIZE;
    ASSERT(page_size > (sizeof(chunk_header_t) + sizeof(block_header_t)),
           "invalid page size");
    ASSERT(IS_POWER_OF_TWO(page_size),
           "invalid page size -- must be a power of two");

    system_info.page_size       = page_size;
    system_info.log_2_page_size = LOG2_64BIT(page_size);

    LOG("page_size:          %lu\n", system_info.page_size);
    LOG("MAX_SMALL_CHUNK:    %llu\n", MAX_SMALL_CHUNK);
    LOG("DEFAULT_BLOCK_SIZE: %llu\n", DEFAULT_BLOCK_SIZE);

    LOG("initialized system info\n");
}

HMALLOC_ALWAYS_INLINE
internal inline void * get_pages_from_os(u64 n_pages, u64 alignment) {
    return alloc_aligned_pages(n_pages, alignment);
}

HMALLOC_ALWAYS_INLINE
internal inline void release_pages_to_os(void *addr, u64 n_pages) {
    (void)n_pages;
    free_pages(addr);
}

HMALLOC_ALWAYS_INLINE
internal inline u32 os_get_tid(void) {
    u32 tid;

    tid = 0;

    return tid;
}

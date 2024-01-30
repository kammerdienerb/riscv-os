#include "kmalloc.h"
#include "hmalloc/hmalloc.c"

#include "utils.h"

void init_kmalloc(void) {
    hmalloc_init();
}

void *kmalloc(u64 n_bytes) {
    return hmalloc_malloc(n_bytes);
}

void *kmalloc_aligned(u64 n_bytes, u64 alignment) {
    return hmalloc_aligned_alloc(alignment, n_bytes);
}

void kfree(void *addr) {
    hmalloc_free(addr);
}

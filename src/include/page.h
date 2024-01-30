#ifndef __PAGE_H__
#define __PAGE_H__

#include "common.h"

void  init_page_allocator(void);
void *alloc_pages(u64 n);
void *alloc_aligned_pages(u64 n, u64 alignment);
void  free_pages(void *pages);

#endif

#ifndef __KMALLOC_H__
#define __KMALLOC_H__

#include "common.h"

void  init_kmalloc(void);
void *kmalloc(u64 n_bytes);
void *kmalloc_aligned(u64 n_bytes, u64 alignment);
void  kfree(void *addr);

#endif

#ifndef __INIT_H__
#define __INIT_H__

#include "internal.h"

internal Spinlock hmalloc_init_lock;
#define INIT_LOCK()   do { spin_lock(&hmalloc_init_lock);   } while (0)
#define INIT_UNLOCK() do { spin_unlock(&hmalloc_init_lock); } while (0)

internal int hmalloc_is_initialized = 0;


internal void hmalloc_init(void);

#endif

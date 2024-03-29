#ifndef __THREAD_H__
#define __THREAD_H__

#include "internal.h"
#include "heap.h"

/*
 * @TODO
 *
 * HMALLOC_MAX_THREADS should be something that's configured
 * at run time -- either through an environment variable,
 * or computed on initialization by checking the number of
 * threads on the system.
 *                                                -- Brandon
 */

#ifndef HMALLOC_MAX_THREADS
#define HMALLOC_MAX_THREADS (1024ULL)
#endif

#if HMALLOC_MAX_THREADS > (1 << 16)
    #error "Can't represent this many threads."
#endif

#if !IS_POWER_OF_TWO_PP(HMALLOC_MAX_THREADS)
    #error "HMALLOC_MAX_THREADS must be a power of two!"
#endif

#define LOG_2_HMALLOC_MAX_THREADS (LOG2_64BIT(HMALLOC_MAX_THREADS))

typedef u16 hm_tid_t;

typedef struct {
    heap_t   heap;
    hm_tid_t tid;
    int      is_valid;
    u64      id;
    u32      ref_count;
} thread_data_t;


internal void threads_init(void);
internal thread_data_t * get_this_thread(void);

internal heap_t * get_this_thread_heap(void);

internal hm_tid_t get_this_tid(void);

#define OS_TID_TO_HM_TID(os_tid) ((os_tid) & (HMALLOC_MAX_THREADS - 1))

#endif

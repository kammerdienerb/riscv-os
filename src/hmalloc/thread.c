#include "internal.h"
#include "thread.h"
#include "heap.h"
#include "os.h"
#include "init.h"
#include "machine.h"

internal thread_data_t thread_datas[MAX_HARTS];

internal void threads_init(void) {
    LOG("initialized threads\n");
}

HMALLOC_ALWAYS_INLINE
internal inline hm_tid_t get_this_tid(void) {
    return OS_TID_TO_HM_TID(os_get_tid());
}

HMALLOC_ALWAYS_INLINE
internal inline void setup_thread(thread_data_t *thr, hm_tid_t tid) {

    /* Ensure our system is initialized. */
    hmalloc_init();

    heap_make(&thr->heap);
    thr->heap.__meta.flags |= HEAP_THREAD;
    thr->is_valid           = 1;
    thr->id                 = tid;
    LOG("initialized a new thread with id %llu\n", thr->id);
}

HMALLOC_ALWAYS_INLINE
internal inline thread_data_t * get_this_thread(void) {
    hm_tid_t       tid;
    thread_data_t *thr;

    tid = get_this_tid();
    thr = thread_datas + tid;

    if (!thr->is_valid) { setup_thread(thr, tid); }

    return thr;
}

HMALLOC_ALWAYS_INLINE
internal inline heap_t * get_this_thread_heap(void) {
    return &get_this_thread()->heap;
}

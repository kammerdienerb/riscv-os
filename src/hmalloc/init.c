#include "init.h"
#include "internal_malloc.h"
#include "os.h"
#include "thread.h"

internal void perform_sanity_checks(void) {
    chunk_header_t c;
    (void)c;

    ASSERT(sizeof(c) == 8, "chunk_header_t is invalid");
    ASSERT(IS_ALIGNED(sizeof(block_header_t), 8), "block header is misaligned");
#ifdef HMALLOC_USE_SBLOCKS
    ASSERT(sizeof(__sblock_slot_nano_t)   == SBLOCK_CLASS_NANO,   "sized sblock type is incorrect");
    ASSERT(sizeof(__sblock_slot_micro_t)  == SBLOCK_CLASS_MICRO,  "sized sblock type is incorrect");
    ASSERT(sizeof(__sblock_slot_tiny_t)   == SBLOCK_CLASS_TINY,   "sized sblock type is incorrect");
    ASSERT(sizeof(__sblock_slot_small_t)  == SBLOCK_CLASS_SMALL,  "sized sblock type is incorrect");
    ASSERT(sizeof(__sblock_slot_medium_t) == SBLOCK_CLASS_MEDIUM, "sized sblock type is incorrect");
    ASSERT(sizeof(__sblock_slot_large_t)  == SBLOCK_CLASS_LARGE,  "sized sblock type is incorrect");
    ASSERT(sizeof(__sblock_slot_huge_t)   == SBLOCK_CLASS_HUGE,   "sized sblock type is incorrect");
    ASSERT(sizeof(__sblock_slot_mega_t)   == SBLOCK_CLASS_MEGA,   "sized sblock type is incorrect");
#endif
}

internal void hmalloc_init(void) {
    /*
     * Thread-unsafe check for performance.
     * Could give a false positive.
     */
    if (unlikely(!hmalloc_is_initialized)) {
        INIT_LOCK(); {
            /* Thread-safe check. */
            if (unlikely(hmalloc_is_initialized)) {
                INIT_UNLOCK();
                return;
            }

            perform_sanity_checks();

#ifdef HMALLOC_DO_LOGGING
            log_init();
#endif
            system_info_init();

            LOG("main thread has tid %d\n", get_this_tid());

            imalloc_init();

            threads_init();

            hmalloc_is_initialized = 1;
        } INIT_UNLOCK();
    }
}

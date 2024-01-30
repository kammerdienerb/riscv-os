#ifndef __HEAP_H__
#define __HEAP_H__

#include "internal.h"

#define DEFAULT_BLOCK_SIZE KB(16)

/* Chunk header flags */
#define CHUNK_IS_FREE      (UINT16_C(0x0001))
#define CHUNK_IS_BIG       (UINT16_C(0x0002))

typedef union {
    struct {
        u64 offset_prev_words  : 20;
        u64 offset_next_words  : 20;
        u64 size               : 21;
        u64 flags              : 3;
    };
    u64 __header;
} chunk_header_t;


typedef struct sblock_header {
    struct sblock_header *prev;
    struct sblock_header *next;
    void                 *end;
    u64                   regions_bitfield;
    u64                   slots_bitfields[64];
    u32                   n_allocations;
    u32                   max_allocations;
    u32                   size_class_idx;
    u32                   size_class;
} sblock_header_t;



#define SBLOCK_N_SIZE_CLASSES (8)

#define SBLOCK_INTERVAL (16ULL)

#define SBLOCK_CLASS_NANO   (SBLOCK_INTERVAL)
#define SBLOCK_CLASS_MICRO  (SBLOCK_CLASS_NANO   + SBLOCK_INTERVAL)
#define SBLOCK_CLASS_TINY   (SBLOCK_CLASS_MICRO  + SBLOCK_INTERVAL)
#define SBLOCK_CLASS_SMALL  (SBLOCK_CLASS_TINY   + SBLOCK_INTERVAL)
#define SBLOCK_CLASS_MEDIUM (SBLOCK_CLASS_SMALL  + SBLOCK_INTERVAL)
#define SBLOCK_CLASS_LARGE  (SBLOCK_CLASS_MEDIUM + SBLOCK_INTERVAL)
#define SBLOCK_CLASS_HUGE   (SBLOCK_CLASS_LARGE  + SBLOCK_INTERVAL)
#define SBLOCK_CLASS_MEGA   (SBLOCK_CLASS_HUGE   + SBLOCK_INTERVAL)

#define SBLOCK_CLASS_NANO_IDX   (0)
#define SBLOCK_CLASS_MICRO_IDX  (1)
#define SBLOCK_CLASS_TINY_IDX   (2)
#define SBLOCK_CLASS_SMALL_IDX  (3)
#define SBLOCK_CLASS_MEDIUM_IDX (4)
#define SBLOCK_CLASS_LARGE_IDX  (5)
#define SBLOCK_CLASS_HUGE_IDX   (6)
#define SBLOCK_CLASS_MEGA_IDX   (7)

typedef struct { char __bytes[SBLOCK_CLASS_NANO];   } __sblock_slot_nano_t;
typedef struct { char __bytes[SBLOCK_CLASS_MICRO];  } __sblock_slot_micro_t;
typedef struct { char __bytes[SBLOCK_CLASS_TINY];   } __sblock_slot_tiny_t;
typedef struct { char __bytes[SBLOCK_CLASS_SMALL];  } __sblock_slot_small_t;
typedef struct { char __bytes[SBLOCK_CLASS_MEDIUM]; } __sblock_slot_medium_t;
typedef struct { char __bytes[SBLOCK_CLASS_LARGE];  } __sblock_slot_large_t;
typedef struct { char __bytes[SBLOCK_CLASS_HUGE];   } __sblock_slot_huge_t;
typedef struct { char __bytes[SBLOCK_CLASS_MEGA];   } __sblock_slot_mega_t;

#define SBLOCK_SMALLEST_CLASS (SBLOCK_CLASS_NANO)
#define SBLOCK_LARGEST_CLASS  (SBLOCK_CLASS_MEGA)

#define SBLOCK_CLASS_NANO_BLOCK_SIZE   (4096ULL * SBLOCK_CLASS_NANO)
#define SBLOCK_CLASS_MICRO_BLOCK_SIZE  (4096ULL * SBLOCK_CLASS_MICRO)
#define SBLOCK_CLASS_TINY_BLOCK_SIZE   (4096ULL * SBLOCK_CLASS_TINY)
#define SBLOCK_CLASS_SMALL_BLOCK_SIZE  (4096ULL * SBLOCK_CLASS_SMALL)
#define SBLOCK_CLASS_MEDIUM_BLOCK_SIZE (4096ULL * SBLOCK_CLASS_MEDIUM)
#define SBLOCK_CLASS_LARGE_BLOCK_SIZE  (4096ULL * SBLOCK_CLASS_LARGE)
#define SBLOCK_CLASS_HUGE_BLOCK_SIZE   (4096ULL * SBLOCK_CLASS_HUGE)
#define SBLOCK_CLASS_MEGA_BLOCK_SIZE   (4096ULL * SBLOCK_CLASS_MEGA)

#ifdef HMALLOC_USE_SBLOCKS
internal u64 _sblock_block_size_lookup[] = {
    SBLOCK_CLASS_NANO_BLOCK_SIZE,
    SBLOCK_CLASS_MICRO_BLOCK_SIZE,
    SBLOCK_CLASS_TINY_BLOCK_SIZE,
    SBLOCK_CLASS_SMALL_BLOCK_SIZE,
    SBLOCK_CLASS_MEDIUM_BLOCK_SIZE,
    SBLOCK_CLASS_LARGE_BLOCK_SIZE,
    SBLOCK_CLASS_HUGE_BLOCK_SIZE,
    SBLOCK_CLASS_MEGA_BLOCK_SIZE,
};
#endif /* HMALLOC_USE_SBLOCKS */

#define SBLOCK_CLASS_NANO_RESERVED_SLOTS   ((4096ULL + (SBLOCK_CLASS_NANO   - 1)) / SBLOCK_CLASS_NANO)
#define SBLOCK_CLASS_MICRO_RESERVED_SLOTS  ((4096ULL + (SBLOCK_CLASS_MICRO  - 1)) / SBLOCK_CLASS_MICRO)
#define SBLOCK_CLASS_TINY_RESERVED_SLOTS   ((4096ULL + (SBLOCK_CLASS_TINY   - 1)) / SBLOCK_CLASS_TINY)
#define SBLOCK_CLASS_SMALL_RESERVED_SLOTS  ((4096ULL + (SBLOCK_CLASS_SMALL  - 1)) / SBLOCK_CLASS_SMALL)
#define SBLOCK_CLASS_MEDIUM_RESERVED_SLOTS ((4096ULL + (SBLOCK_CLASS_MEDIUM - 1)) / SBLOCK_CLASS_MEDIUM)
#define SBLOCK_CLASS_LARGE_RESERVED_SLOTS  ((4096ULL + (SBLOCK_CLASS_LARGE  - 1)) / SBLOCK_CLASS_LARGE)
#define SBLOCK_CLASS_HUGE_RESERVED_SLOTS   ((4096ULL + (SBLOCK_CLASS_HUGE   - 1)) / SBLOCK_CLASS_HUGE)
#define SBLOCK_CLASS_MEGA_RESERVED_SLOTS   ((4096ULL + (SBLOCK_CLASS_MEGA   - 1)) / SBLOCK_CLASS_MEGA)

#ifdef HMALLOC_USE_SBLOCKS
internal u64 _sblock_reserved_slots_lookup[] = {
    SBLOCK_CLASS_NANO_RESERVED_SLOTS,
    SBLOCK_CLASS_MICRO_RESERVED_SLOTS,
    SBLOCK_CLASS_TINY_RESERVED_SLOTS,
    SBLOCK_CLASS_SMALL_RESERVED_SLOTS,
    SBLOCK_CLASS_MEDIUM_RESERVED_SLOTS,
    SBLOCK_CLASS_LARGE_RESERVED_SLOTS,
    SBLOCK_CLASS_HUGE_RESERVED_SLOTS,
    SBLOCK_CLASS_MEGA_RESERVED_SLOTS,
};
#endif /* HMALLOC_USE_SBLOCKS */

#define SBLOCK_MAX_ALLOC_SIZE (SBLOCK_LARGEST_CLASS)


struct cblock_list;

typedef struct cblock_header {
    chunk_header_t       *free_list_head,
                         *free_list_tail;
    struct cblock_header *prev;
    struct cblock_header *next;
    void                 *end;
    struct cblock_list   *list;
} cblock_header_t;


typedef struct cblock_list {
    cblock_header_t *head;
    cblock_header_t *tail;
    Spinlock         lock;
} cblock_list_t;

#define LIST_LOCK_INIT(l) ((l)->lock.s = SPIN_UNLOCKED)
#define LIST_LOCK(l)      spin_lock(&(l)->lock)
#define LIST_UNLOCK(l)    spin_unlock(&(l)->lock)

#define N_SIZE_CLASSES (5)

#ifdef HMALLOC_USE_SBLOCKS
#define CLASS_MICRO    (SBLOCK_LARGEST_CLASS << 1ULL)
#else
#define CLASS_MICRO    (64ULL)
#endif

#define CLASS_SMALL    (CLASS_MICRO  << 2ULL)
#define CLASS_MEDIUM   (CLASS_SMALL  << 2ULL)
#define CLASS_LARGE    (CLASS_MEDIUM << 2ULL)
#define CLASS_HUGE     (CLASS_LARGE  << 2ULL)


#define SMALLEST_CLASS (CLASS_MICRO)
#define LARGEST_CLASS  (CLASS_HUGE)

/*
** This value effects how small chunks can get when they're split
** (which also dictates the smallest amount of memory we can dish
** out from cblocks).
** Values smaller than 128 (powers of two) were causing orted to
** crash in a strange way that I couldn't really figure out, but
** using a value of at least 128 seemed to fix it...
**
**                                           Brandon Kammerdiener
**                                                    April, 2021
*/
#ifdef HMALLOC_USE_SBLOCKS
#define CHUNK_MIN_SIZE   (SBLOCK_LARGEST_CLASS + 1ULL)
#else
#define CHUNK_MIN_SIZE   (128ULL)
#endif

#define MAX_SMALL_CHUNK  (DEFAULT_BLOCK_SIZE - sizeof(block_header_t) - sizeof(chunk_header_t))



typedef struct {
    void     *addr;
    union {
        char *handle;
        u16   tid;
    };
    u32       hid;
    u16       flags;
} heap__meta_t;


#define BLOCK_KIND_CBLOCK (0x1)
#define BLOCK_KIND_SBLOCK (0x2)

typedef struct {
    union {
        cblock_header_t c;
#ifdef HMALLOC_USE_SBLOCKS
        sblock_header_t s;
#endif
    };
    heap__meta_t        heap__meta;
    u16                 tid;
    u8                  block_kind;
} block_header_t;




#define ADDR_PARENT_BLOCK(addr) \
    ((block_header_t*)(void*)(((u64)(void*)(addr)) & ~(DEFAULT_BLOCK_SIZE - 1)))

#define BLOCK_GET_HEAP_PTR(b) \
    ((heap_t*)((b)->heap__meta.addr))


#define CHUNK_SIZE(addr) (((chunk_header_t*)((void*)(addr)))->size << 3ULL)

#define SET_CHUNK_SIZE(addr, sz) do {                          \
    ASSERT(IS_ALIGNED(sz, 8), "chunk size not aligned");       \
    ((chunk_header_t*)((void*)(addr)))->size = ((sz) >> 3ULL); \
} while (0)

#define SMALL_CHUNK_ADJACENT(addr) \
    ((chunk_header_t*)(((void*)addr) + sizeof(chunk_header_t) + CHUNK_SIZE(addr)))

#define SET_CHUNK_OFFSET_PREV(addr, off) do {                 \
    ASSERT(IS_ALIGNED((off), 8), "offset prev not aligned");  \
    ((chunk_header_t*)((void*)(addr)))->offset_prev_words     \
        = ((off) >> 3ULL);                                    \
} while (0)

#define SET_CHUNK_OFFSET_NEXT(addr, off) do {                 \
    ASSERT(IS_ALIGNED((off), 8), "offset next not aligned");  \
    ((chunk_header_t*)((void*)(addr)))->offset_next_words     \
        = ((off) >> 3ULL);                                    \
} while (0)

#define CHUNK_PREV(addr)  ((chunk_header_t*)(unlikely(((chunk_header_t*)(addr))->offset_prev_words == 0)) \
                             ? NULL                                                                       \
                             :   ((void*)(addr))                                                          \
                               - (((chunk_header_t*)(addr))->offset_prev_words << 3ULL))

#define CHUNK_NEXT(addr)  ((chunk_header_t*)(unlikely(((chunk_header_t*)(addr))->offset_next_words == 0) \
                             ? NULL                                                                      \
                             :   ((void*)(addr))                                                         \
                               + (((chunk_header_t*)(addr))->offset_next_words << 3ULL)))

#define CHUNK_PREV_UNCHECKED(addr)                                \
    (((void*)(addr))                                              \
        - (((chunk_header_t*)(addr))->offset_prev_words << 3ULL))

#define CHUNK_NEXT_UNCHECKED(addr)                                \
    (((void*)(addr))                                              \
        + (((chunk_header_t*)(addr))->offset_next_words << 3ULL))

#define CHUNK_HAS_PREV(addr) (((chunk_header_t*)(addr))->offset_prev_words != 0)
#define CHUNK_HAS_NEXT(addr) (((chunk_header_t*)(addr))->offset_next_words != 0)

#define CHUNK_USER_MEM(addr) (((void*)(addr)) + sizeof(chunk_header_t))

#define CHUNK_FROM_USER_MEM(addr) ((chunk_header_t*)(((void*)(addr)) - sizeof(chunk_header_t)))

#define CHUNK_DISTANCE(a, b) (((void*)(a)) - (((void*)(b))))

#define CHUNK_PARENT_BLOCK(addr) \
    ADDR_PARENT_BLOCK(addr)

#define CBLOCK_FIRST_CHUNK(addr) ALIGN((((void*)(addr)) + sizeof(block_header_t)), 8)

#define LARGEST_CHUNK_IN_EMPTY_N_PAGE_BLOCK(N) \
    (((N) << system_info.log_2_page_size) - sizeof(block_header_t) - sizeof(chunk_header_t))



#define HEAP_THREAD (0x1)
#define HEAP_USER   (0x2)

internal u32 hid_counter;

typedef struct {
    cblock_list_t       lists[N_SIZE_CLASSES];
#ifdef HMALLOC_USE_SBLOCKS
    Spinlock            s_locks[SBLOCK_N_SIZE_CLASSES];
    u32                 n_sblocks[SBLOCK_N_SIZE_CLASSES];
    sblock_header_t    *sblocks_heads[SBLOCK_N_SIZE_CLASSES],
                       *sblocks_tails[SBLOCK_N_SIZE_CLASSES];
#endif
    heap__meta_t        __meta;
} heap_t;

internal void heap_make(heap_t *heap);
internal void * heap_alloc(heap_t *heap, u64 n_bytes);

#ifdef HMALLOC_USE_SBLOCKS
#define HEAP_S_LOCK_INIT(heap_ptr, idx) (heap_ptr->s_locks[(idx)].s = SPIN_UNLOCKED)
#define HEAP_S_LOCK(heap_ptr, idx)      spin_lock(&heap_ptr->s_locks[(idx)])
#define HEAP_S_UNLOCK(heap_ptr, idx)    spin_unlock(&heap_ptr->s_locks[(idx)])
#endif

#endif

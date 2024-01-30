#include "page.h"
#include "machine.h"
#include "lock.h"
#include "symbols.h"
#include "utils.h"
#include "kprint.h"


static Spinlock page_lock;


static inline void *get_page(u32 i)          { return (void*)(sym_start(heap) + (i * PAGE_SIZE));  }
static inline u64   get_page_idx(void *page) { return (((u64)page) - sym_start(heap)) / PAGE_SIZE; }


typedef struct {
    u64 n_pages;
    u64 n_free;
    u64 last_place;
    u32 n_reserved;
} Page_Status;

static Page_Status *pagestatus;

#define PAGESTATUS_NUM_BYTES ((pagestatus->n_pages + 7) >> 3)
#define TAKEN_BYTES          ((u8*)(((void*)pagestatus) + sizeof(*pagestatus)))
#define LAST_BYTES           ((u8*)(((void*)pagestatus) + sizeof(*pagestatus) + PAGESTATUS_NUM_BYTES))

static inline void set_taken(u64 i, u8 val) {
    u64 byte;
    u32 shift;

    byte  = i >> 3;
    shift = 7 - (i & 7);

    TAKEN_BYTES[byte] &= ~(1 << shift);
    TAKEN_BYTES[byte] |= ((val & 1) << shift);
}

static inline u32 is_taken(u64 i) {
    u64 byte;
    u32 shift;

    byte  = i >> 3;
    shift = 7 - (i & 7);

    return TAKEN_BYTES[byte] & (1 << shift);
}

static inline void set_last(u64 i, u8 val) {
    u64 byte;
    u32 shift;

    byte  = i >> 3;
    shift = 7 - (i & 7);

    LAST_BYTES[byte] &= ~(1 << shift);
    LAST_BYTES[byte] |= ((val & 1) << shift);
}

static inline u32 is_last(u64 i) {
    u64 byte;
    u32 shift;

    byte  = i >> 3;
    shift = 7 - (i & 7);

    return LAST_BYTES[byte] & (1 << shift);
}

void init_page_allocator(void) {
    u64 i;

    spin_lock(&page_lock);

    pagestatus             = (void*)sym_start(heap);
    pagestatus->n_pages    = (sym_end(heap) - sym_start(heap)) / PAGE_SIZE;
    pagestatus->n_reserved = (ALIGN(((void*)LAST_BYTES) + PAGESTATUS_NUM_BYTES, PAGE_SIZE) - (void*)pagestatus) / PAGE_SIZE;
    pagestatus->n_free     = pagestatus->n_pages - pagestatus->n_reserved;
    pagestatus->last_place = pagestatus->n_reserved;

    memset(TAKEN_BYTES, 0, PAGESTATUS_NUM_BYTES);
    memset(LAST_BYTES,  0, PAGESTATUS_NUM_BYTES);

    for (i = 0; i < pagestatus->n_reserved; i += 1) { set_taken(i, 1); }

    spin_unlock(&page_lock);
}

void *alloc_aligned_pages(u64 n, u64 alignment) {
    void *page;
    u64   n_found;
    u64   cursor;
    u64   p;

    page = NULL;

    spin_lock(&page_lock);

    (void)alignment;

    if (pagestatus->n_free == 0) {
        kprint("no free pages to allocate\n");
        goto out_unlock;
    }

    n_found = 0;

    cursor = pagestatus->last_place;
    while (cursor < pagestatus->n_pages) {
        n_found = is_taken(cursor) ? 0 : n_found + 1;

        if (n_found >= n
        &&  IS_ALIGNED(get_page(cursor - n + 1), alignment)) {

            for (p = cursor; p > cursor - n; p -= 1) { set_taken(p, 1); set_last(p, 0); }
            set_last(cursor, 1);
            pagestatus->last_place = cursor + 1;
            pagestatus->n_free -= n;
            page = get_page(cursor - n + 1);
            goto out_unlock;
        }

        cursor += 1;
    }

    /* Search again from start. */
    cursor = pagestatus->last_place = pagestatus->n_reserved;
    while (cursor < pagestatus->n_pages) {
        n_found = is_taken(cursor) ? 0 : n_found + 1;

        if (n_found >= n
        &&  IS_ALIGNED(get_page(cursor - n + 1), alignment)) {

            for (p = cursor; p > cursor - n; p -= 1) { set_taken(p, 1); set_last(p, 0); }
            set_last(cursor, 1);
            pagestatus->last_place = cursor + 1;
            pagestatus->n_free -= n;
            page = get_page(cursor - n + 1);
            goto out_unlock;
        }

        cursor += 1;
    }

    kprint("failed to allocate page(s)\n");

out_unlock:;
    spin_unlock(&page_lock);
    return page;
}

void *alloc_pages(u64 n) { return alloc_aligned_pages(n, PAGE_SIZE); }

void free_pages(void *pages) {
    u64 i;

    spin_lock(&page_lock);

    if (!IS_ALIGNED(pages, PAGE_SIZE)
    ||  (u64)pages <  sym_start(heap)
    ||  (u64)pages >= sym_end(heap)) {
        kprint("attempt to free invalid page(s) at 0x%X\n", pages);
        goto out_unlock;
    }

    i = get_page_idx(pages);

    do {
        set_taken(i, 0);
        pagestatus->n_free += 1;
    } while (!is_last(i++));

out_unlock:;
    spin_unlock(&page_lock);
}

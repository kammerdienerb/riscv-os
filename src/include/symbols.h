#ifndef __SYMBOLS_H__
#define __SYMBOLS_H__

#include "common.h"

extern void *_heap_start;
extern void *_heap_end;
extern void *_stack_start;
extern void *_stack_end;
extern void *_bss_start;
extern void *_bss_end;
extern void *_data_start;
extern void *_data_end;
extern void *_text_start;
extern void *_text_end;
extern void *_rodata_start;
extern void *_rodata_end;
extern void *_memory_start;
extern void *_memory_end;

#define sym_start(segment) \
    ((u64)&_##segment##_start)
#define sym_end(segment) \
    ((u64)&_##segment##_end)
#define sym_size(segment) \
    (sym_end(segment) - sym_start(segment))

#endif

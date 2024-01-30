#ifndef __HART_MNG_H__
#define __HART_MNG_H__

#include "common.h"
#include "machine.h"
#include "lock.h"

typedef struct {
    struct {
        u64 target_addr;
        u64 scratch;
    } start;
    Spinlock lock;
    u8       status;
} Hart_Info;

extern Hart_Info harts[MAX_HARTS];

s64  start_hart(u32 hart, u64 start_addr, u64 scratch);
s64  stop_hart(u32 hart);
s64  hart_handle_msip(u32 hart);
void clint_awaken_hart(u32 hart);
void clint_reset_msip(u32 hart);


#endif

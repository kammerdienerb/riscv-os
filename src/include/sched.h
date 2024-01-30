#ifndef __SCHED_H__
#define __SCHED_H__

#include "common.h"
#include "lock.h"
#include "tree.h"
#include "process.h"

typedef Process *process_ptr_t;

use_tree(u64, process_ptr_t);

typedef struct {
    u32                       hart;
    Spinlock                  lock;
    tree(u64, process_ptr_t)  procs;
    Process                  *current;
    Process                  *idle;
    u64                       last_tick_time;
} Scheduler;

extern Scheduler scheds[MAX_HARTS];
extern u32       sched_online;

void init_sched(void);
void sched_tick(u32 hartid);
Process *sched_current(u32 hartid);
void sched_add(Process *proc);
void sched_add_on_hart(Process *proc, u32 which_hart);
void sched_exit_current(s64 exit_code);
void sched_sleep_current(u64 n_ticks);
void sched_wait_current(u32 waiting_on);
void sched_try_wake(u32 waiting_on);
s32  get_idle_hart(void);

#endif

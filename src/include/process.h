#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "common.h"
#include "machine.h"
#include "mmu.h"
#include "lock.h"

#define MAX_PROCS (32)

typedef struct {
    u64    gpregs[32];
    double fpregs[32];
	u64    sepc;
	u64    sstatus;
	u64    sie;
	u64    satp;
	u64    sscratch;
    u64    stvec;
    u64    trap_satp;
    u64    trap_stack;
} Process_Frame;

enum {
    PROC_INVALID,
    PROC_SLEEPING,
    PROC_WAITING,
    PROC_RUNNABLE,
    PROC_RUNNING,
};

enum {
    PROC_WAIT_NONE,
    PROC_WAIT_INPUT,
};

enum {
    PROC_KERNEL,
    PROC_USER,
    PROC_IDLE,
};

typedef struct {
    Process_Frame  frame;
    u32            state;
    s32            on_hart;
    u32            idx;
    u64            sched_time;
    u64            vruntime;
    u32            kind;
    u16            pid;
    Page_Table    *page_table;
    u32            stack_pages;
    void          *stack;
    u64            sleep_cycles;
    u32            waiting_on;
    void          *image;
    u64            virt_avail;
} Process;

extern u16      pid_count;
extern Process  procs[MAX_PROCS];
extern Spinlock procs_lock;

void idle_process_fn(void);
Process * new_process(u32 kind);
void free_process(Process *proc);
void start_process(Process *proc, u32 which_hart);

#endif

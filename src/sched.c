#include "sched.h"
#include "process.h"
#include "tree.h"
#include "lock.h"
#include "sbi.h"
#include "tick.h"
#include "kprint.h"

Scheduler scheds[MAX_HARTS];
u32       sched_online;

static void start_proc(Scheduler *sched, Process *proc) {
    if (proc == NULL) {
        kprint("%rTrying to schedule a NULL process!%_\n");
        for (;;) { WAIT_FOR_INTERRUPT(); }
    }

    proc->sched_time = sbicall(SBI_CLOCK);
    proc->state      = PROC_RUNNING;
    proc->on_hart    = sched->hart;
    start_process(proc, sched->hart);
}

void init_sched(void) {
    u32 hart;

    /* Only set up process and scheduling for harts > 0
     * so that we maintain the kernel console. */

    for (hart = 1; hart < MAX_HARTS; hart += 1) {
        scheds[hart].hart    = hart;
        scheds[hart].procs   = tree_make(u64, process_ptr_t);
        scheds[hart].idle    = new_process(PROC_IDLE);
        scheds[hart].current = scheds[hart].idle;
    }

    for (hart = 1; hart < MAX_HARTS; hart += 1) {
        start_proc(&scheds[hart], scheds[hart].idle);
    }

    sched_online = 1;
}

static Process *pull_next_runnable(Scheduler *sched) {
    tree_it(u64, process_ptr_t)  it;
    Process                     *proc;

    tree_traverse(sched->procs, it) {
        proc = tree_it_val(it);
        if (proc->state == PROC_RUNNABLE) {
            tree_delete(sched->procs, tree_it_key(it));
            return proc;
        }
    }

    return NULL;
}

static void reschedule(Scheduler *sched) {
    Process *next;

    next = pull_next_runnable(sched);

    if (next == NULL) {
        next = sched->idle;
    }

    sched->current = next;
}

static void deschedule_current(Scheduler *sched, u32 new_state) {
    Process *proc;
    u64      vrt;

    proc           = sched->current;
    sched->current = NULL;
    vrt            = proc->vruntime + (sbicall(SBI_CLOCK) - proc->sched_time);

    if (proc->kind == PROC_IDLE) {
        proc->vruntime = vrt;
        proc->state    = new_state;
    } else {
        while (tree_it_good(tree_lookup(sched->procs, vrt))) { vrt += 1; }

        proc->vruntime = vrt;
        proc->state    = new_state;

        tree_insert(sched->procs, proc->vruntime, proc);
    }
}

static void do_schedule(Scheduler *sched) {
    if (sched->hart == 0) { return; }

    spin_lock(&sched->lock);

    if (sched->current != NULL) {
        CSR_READ(sched->current->frame.sepc, "sepc");
    }

    if (tree_len(sched->procs) > 0) {
        deschedule_current(sched, PROC_RUNNABLE);
        reschedule(sched);
    } else {
        sched->current->vruntime += (sbicall(SBI_CLOCK) - sched->current->sched_time);
    }

    spin_unlock(&sched->lock);

    /* Will not return from start_proc()!!! */
    start_proc(sched, sched->current);
}

static void do_sched_tick(Scheduler *sched) {
    u64                          elapsed;
    tree_it(u64, process_ptr_t)  it;
    Process                     *proc;

    if (sched->hart == 0) { return; }

    elapsed = sbicall(SBI_CLOCK) - sched->last_tick_time;
    spin_lock(&sched->lock);

    tree_traverse(sched->procs, it) {
        proc = tree_it_val(it);
        if (proc->state == PROC_SLEEPING) {
            if (elapsed >= proc->sleep_cycles) {
                proc->state = PROC_RUNNABLE;
            } else {
                proc->sleep_cycles -= elapsed;
            }
        }
    }

    spin_unlock(&sched->lock);

    do_schedule(sched);

    sched->last_tick_time = sbicall(SBI_CLOCK);
}

void sched_tick(u32 hartid) {
    if (!sched_online) { return; }

    do_sched_tick(scheds + hartid);
}

Process *sched_current(u32 hartid) {
    if (!sched_online) { return NULL; }

    return (scheds + hartid)->current;
}

void sched_add(Process *proc) {
    if (!sched_online) { return; }

    sched_add_on_hart(proc, sbicall(SBI_HART_ID));
}

void sched_add_on_hart(Process *proc, u32 which_hart) {
    Scheduler *sched;
    u64        vrt;

    if (!sched_online) { return; }

    sched = &scheds[which_hart];

    spin_lock(&sched->lock);

    vrt = 0;

    while (tree_it_good(tree_lookup(sched->procs, vrt))) { vrt += 1; }

    proc->vruntime = vrt;
    proc->state    = PROC_RUNNABLE;
    proc->on_hart  = which_hart;

    tree_insert(sched->procs, proc->vruntime, proc);

    spin_unlock(&sched->lock);
}

void sched_exit_current(s64 exit_code) {
    u32        hart;
    Scheduler *sched;

    if (!sched_online) { return; }

    (void)exit_code;

    hart = sbicall(SBI_HART_ID);

    sched = &scheds[hart];

    if (sched->current       == NULL
    ||  sched->current->kind == PROC_IDLE) {

        return;
    }

    spin_lock(&sched->lock);

    free_process(sched->current);
    sched->current = NULL;
    reschedule(sched);

    spin_unlock(&sched->lock);

    /* Will not return from start_proc()!!! */
    start_proc(sched, sched->current);
}

void sched_sleep_current(u64 n_cycles) {
    u32        hart;
    Scheduler *sched;

    if (!sched_online) { return; }

    hart = sbicall(SBI_HART_ID);

    sched = &scheds[hart];

    if (sched->current       == NULL
    ||  sched->current->kind == PROC_IDLE) {

        return;
    }

    spin_lock(&sched->lock);

    sched->current->sleep_cycles = n_cycles;
    deschedule_current(sched, PROC_SLEEPING);
    reschedule(sched);

    spin_unlock(&sched->lock);

    /* Will not return from start_proc()!!! */
    start_proc(sched, sched->current);
}

void sched_wait_current(u32 waiting_on) {
    u32        hart;
    Scheduler *sched;

    if (!sched_online) { return; }

    hart = sbicall(SBI_HART_ID);

    sched = &scheds[hart];

    if (sched->current       == NULL
    ||  sched->current->kind == PROC_IDLE) {

        return;
    }

    spin_lock(&sched->lock);

    sched->current->waiting_on = waiting_on;
    deschedule_current(sched, PROC_WAITING);
    reschedule(sched);

    spin_unlock(&sched->lock);

    /* Will not return from start_proc()!!! */
    start_proc(sched, sched->current);
}

void sched_try_wake(u32 waiting_on) {
    u32                          hart;
    Scheduler                   *sched;
    u32                          woken;
    tree_it(u64, process_ptr_t)  it;
    Process                     *proc;

    if (!sched_online) { return; }

    for (hart = 1; hart < MAX_HARTS; hart += 1) {
        sched = &scheds[hart];
        woken = 0;

        if (sched->current == sched->idle) {
            spin_lock(&sched->lock);

            tree_traverse(sched->procs, it) {
                proc = tree_it_val(it);
                if (proc->state      == PROC_WAITING
                &&  proc->waiting_on == waiting_on) {
                    proc->waiting_on = PROC_WAIT_NONE;
                    proc->state      = PROC_RUNNABLE;
                    woken            = 1;
                }
            }

            spin_unlock(&sched->lock);

            if (woken) {
                force_tick(hart);
            }
        }
    }
}

s32 get_idle_hart(void) {
    u32        hart;
    Scheduler *sched;

    if (!sched_online) { return -1; }

    for (hart = 1; hart < MAX_HARTS; hart += 1) {
        sched = &scheds[hart];
        if (sched->current == sched->idle) {
            return hart;
        }
    }

    return -1;
}

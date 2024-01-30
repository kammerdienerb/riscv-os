#include "process.h"
#include "kprint.h"
#include "page.h"
#include "mmu.h"
#include "sbi.h"
#include "symbols.h"
#include "kmalloc.h"
#include "lock.h"
#include "utils.h"

void trap_jump(void);

void idle_process_fn(void) {
    for (;;) { WAIT_FOR_INTERRUPT(); }
}

extern u64 spawn_trap_start;
extern u64 spawn_trap_size;
extern u64 spawn_thread_start;
extern u64 spawn_thread_size;

u16      pid_count = 1;
Process  procs[MAX_PROCS];
Spinlock procs_lock;

Process * new_process(u32 kind) {
    u32      idx;
    Process *proc;
    u32      asid;
    u64      phys_addr;
    u64      page_paddr;
    u64      page_vaddr;
    u64      stack_paddr;
    u64      stack_vaddr;

    spin_lock(&procs_lock);

    for (idx = 0; idx < MAX_PROCS; idx += 1) {
        if (procs[idx].state == PROC_INVALID) {
            goto found;
        }
    }

    spin_unlock(&procs_lock);

    kprint("no more proc slots\n");
    return NULL;

found:;

    proc = procs + idx;

    memset(proc, 0, sizeof(*proc));

    proc->pid        = pid_count;
    proc->kind       = kind;
    proc->state      = PROC_RUNNABLE;
    proc->idx        = idx;
    proc->sched_time = 0;
    proc->vruntime   = 0;
    proc->virt_avail = 0x70000000;

    pid_count += 1;

    spin_unlock(&procs_lock);

    if (kind == PROC_USER) {
        proc->page_table = alloc_pages(1);

        asid = proc->pid;

        phys_addr  = virt_to_phys(kernel_pt, spawn_thread_start);
        page_paddr = phys_addr & ~(PAGE_SIZE - 1);
        page_vaddr = ((u64)spawn_thread_start) & ~(PAGE_SIZE - 1);

        mmu_map(proc->page_table,
                page_paddr, page_vaddr,
                ALIGN((spawn_thread_start + spawn_thread_size) - page_vaddr, PAGE_SIZE),
                PAGE_EXECUTE);

        phys_addr  = virt_to_phys(kernel_pt, spawn_trap_start);
        page_paddr = phys_addr & ~(PAGE_SIZE - 1);
        page_vaddr = ((u64)spawn_trap_start) & ~(PAGE_SIZE - 1);

        mmu_map(proc->page_table,
                page_paddr, page_vaddr,
                ALIGN((spawn_trap_start + spawn_trap_size) - page_vaddr, PAGE_SIZE),
                PAGE_EXECUTE);

        phys_addr  = virt_to_phys(kernel_pt, (u64)&proc->frame);
        page_paddr = phys_addr & ~(PAGE_SIZE - 1);
        page_vaddr = ((u64)&proc->frame) & ~(PAGE_SIZE - 1);

        mmu_map(proc->page_table,
                page_paddr, page_vaddr,
                ALIGN((((u64)&proc->frame) + sizeof(proc->frame)) - page_vaddr, PAGE_SIZE),
                PAGE_READ | PAGE_WRITE);

        proc->stack_pages = 4;

        stack_paddr = ((u64)alloc_pages(proc->stack_pages));
        stack_vaddr = 0x10000000;
        proc->stack = (void*)stack_paddr;

        mmu_map(proc->page_table,
                stack_paddr, stack_vaddr,
                proc->stack_pages * PAGE_SIZE,
                PAGE_USER | PAGE_READ | PAGE_WRITE);

    } else if (kind == PROC_KERNEL || kind == PROC_IDLE) {
        proc->page_table  = kernel_pt;
        asid              = KERNEL_ASID;
        proc->stack_pages = 2;
        stack_vaddr       = (u64)kmalloc_aligned(proc->stack_pages * PAGE_SIZE, PAGE_SIZE);
        proc->stack       = (void*)stack_vaddr;
    }

    if (kind == PROC_IDLE) {
        proc->frame.sepc = (u64)idle_process_fn;
    }

    proc->frame.sstatus         = SSTATUS_FS(1) | SSTATUS_SPIE | SSTATUS_SPP(kind == PROC_USER ? USER_MODE : SUPERVISOR_MODE);
    proc->frame.sie             = SIE_SEIE | SIE_SSIE | SIE_STIE;
    proc->frame.satp            = SATP_MODE_SV39 | SATP_SET_PPN(proc->page_table) | SATP_SET_ASID(asid);
    proc->frame.sscratch        = (u64)&proc->frame;
    proc->frame.stvec           = spawn_trap_start;
    proc->frame.gpregs[XREG_SP] = stack_vaddr + (proc->stack_pages * PAGE_SIZE);
    proc->frame.trap_satp       = SATP_MODE_SV39 | SATP_SET_PPN(kernel_pt) | SATP_SET_ASID(KERNEL_ASID);
    proc->frame.trap_stack      = ((u64)alloc_pages(1)) + PAGE_SIZE;

    return proc;
}

void free_process(Process *proc) {
    if (proc->kind == PROC_USER) {
        free_pages(proc->page_table);
        free_pages(proc->stack);
        kfree(proc->image);
    } else if (proc->kind == PROC_KERNEL || proc->kind == PROC_IDLE) {
        kfree(proc->stack);
    }
    free_pages((void*)(proc->frame.trap_stack - PAGE_SIZE));
}

void start_process(Process *proc, u32 which_hart) {
    u64 phys;

    if (which_hart == sbicall(SBI_HART_ID)) {
        CSR_WRITE("sscratch", proc->frame.sscratch);
        ((void(*)(void))spawn_thread_start)();
        return;
    }

    phys = virt_to_phys(kernel_pt, proc->frame.sscratch);

    sbicall(SBI_START_HART, which_hart, spawn_thread_start, phys);
}

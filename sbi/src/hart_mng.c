#include "hart_mng.h"
#include "hart.h"
#include "kprint.h"

void park_hart(void);

Hart_Info harts[MAX_HARTS];

s64 start_hart(u32 hart, u64 start_addr, u64 scratch) {
    Hart_Info *hart_info;

    if (hart >= MAX_HARTS) { return -1; }

    spin_lock(&harts[hart].lock);

    hart_info = harts + hart;

    if (hart_info->status == HART_INVALID
    ||  hart_info->status == HART_STARTED) {

        kprint("%start_hart(): hart %u not startable (status = %u)%_\n", hart, hart_info->status);

        spin_unlock(&harts[hart].lock);
        return -1;
    }

    hart_info->status            = HART_STARTING;
    hart_info->start.target_addr = start_addr;
    hart_info->start.scratch     = scratch;

    clint_awaken_hart(hart);

    spin_unlock(&harts[hart].lock);

    return 0;
}

s64 stop_hart(u32 hart) {
    spin_lock(&harts[hart].lock);

    if (harts[hart].status != HART_STARTED) {
        spin_unlock(&harts[hart].lock);
        return -1;
    }

    harts[hart].status = HART_STOPPED;

    CSR_WRITE("mepc",     park_hart);
    CSR_WRITE("mstatus",  MSTATUS_MPP(MACHINE_MODE) | MSTATUS_MPIE);
    CSR_WRITE("mie",      MIE_MSIE);
    CSR_WRITE("satp",     0);
    CSR_WRITE("stvec",    0);
    CSR_WRITE("sepc",     0);
    CSR_WRITE("sscratch", 0);

    spin_unlock(&harts[hart].lock);

    MRET();

    return 0;
}

void clint_awaken_hart(u32 hart) {
    volatile u32 *clint = (u32*)CLINT_BASE;
    clint[hart] = 1;
}

void clint_reset_msip(u32 hart) {
    volatile u32 *clint = (u32*)CLINT_BASE;
    clint[hart] = 0;
}

s64 hart_handle_msip(u32 hart) {
    spin_lock(&harts[hart].lock);

    clint_reset_msip(hart);

    if (harts[hart].status == HART_STARTING) {
        CSR_WRITE("mepc",     harts[hart].start.target_addr);
        CSR_WRITE("mstatus",  MSTATUS_MPP(SUPERVISOR_MODE) | MSTATUS_MPIE | MSTATUS_FS(1));
        CSR_WRITE("mie",      MIE_MEIE | MIE_SSIE | MIE_STIE | MIE_MTIE);
        CSR_WRITE("mideleg",  SIP_SEIP | SIP_SSIP | SIP_STIP);
        CSR_WRITE("medeleg",  MEDELEG_ALL);
        CSR_WRITE("sscratch", harts[hart].start.scratch);

        harts[hart].status = HART_STARTED;
    }

    spin_unlock(&harts[hart].lock);

    MRET();

    return 0;
}

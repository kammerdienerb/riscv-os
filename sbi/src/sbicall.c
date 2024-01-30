#include "sbi.h"
#include "machine.h"
#include "kprint.h"
#include "trap.h"
#include "uart.h"
#include "hart_mng.h"
#include "timer.h"

s64 handle_SBI_PUTC(u32 hartid, u8 c) {
    uart_putc(c);
    return 0;
}

s64 handle_SBI_GETC(u32 hartid) {
    trap_frame[hartid].gpregs[XREG_A0] = uart_buffered_getc();
    return 0;
}

s64 handle_SBI_HART_ID(u32 hartid) {
    u32 mhartid;
    CSR_READ(mhartid, "mhartid");
    trap_frame[hartid].gpregs[XREG_A0] = mhartid;
    return 0;
}

s64 handle_SBI_HART_STATUS(u32 hartid, u32 which_hart) {
    if (which_hart >= MAX_HARTS) {
        kprint("%rSBI_HART_STATUS: bad hart (%u)%_\n", which_hart);
        return -1;
    }

    trap_frame[hartid].gpregs[XREG_A0] = harts[which_hart].status;

    return 0;
}

s64 handle_SBI_START_HART(u32 hartid, u32 which_hart, u64 start_addr, u32 priv_mode) {
    if (which_hart >= MAX_HARTS) {
        kprint("%rSBI_START_HART: bad hart (%u)%_\n", which_hart);
        return -1;
    }
    return start_hart(which_hart, start_addr, priv_mode);
}

s64 handle_SBI_STOP_HART(u32 hartid) {
    return stop_hart(hartid);
}

s64 handle_SBI_CLOCK(u32 hartid) {
    trap_frame[hartid].gpregs[XREG_A0] = time_now();
    return 0;
}

s64 handle_SBI_TIMER_REL(u32 hartid, u32 which_hart, u64 when) {
    timer_rel(which_hart ,when);
    return 0;
}

s64 handle_SBI_TIMER_ABS(u32 hartid, u32 which_hart, u64 when) {
    timer_abs(which_hart, when);
    return 0;
}

s64 handle_SBI_TIMER_CLEAR(u32 hartid) {
    u64 mip;

    CSR_READ(mip, "mip");
    CSR_WRITE("mip", mip & ~MIP_STIP);

    return 0;
}

typedef s64 (*sbicall_handler_t)();

#define SBICALL_FN(s, d) (sbicall_handler_t)handle_##s,
sbicall_handler_t sbicall_handlers[NUM_SBICALL] = {
    LIST_SBICALL(SBICALL_FN)
};
#undef SBICALL_FN

s64 do_sbicall(u32 hartid, u32 call) {
    if (call >= NUM_SBICALL) {
        kprint("%c[sbicall]%_ %rbad sbicall number (%u)%_\n", call);
        return -1;
    }

    trap_frame[hartid].gpregs[XREG_A0] = 0;

    return sbicall_handlers[call](
            hartid,
            trap_frame[hartid].gpregs[XREG_A1],
            trap_frame[hartid].gpregs[XREG_A2],
            trap_frame[hartid].gpregs[XREG_A3],
            trap_frame[hartid].gpregs[XREG_A4],
            trap_frame[hartid].gpregs[XREG_A5],
            trap_frame[hartid].gpregs[XREG_A6]);
}

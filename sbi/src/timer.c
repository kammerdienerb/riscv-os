#include "timer.h"
#include "machine.h"

static u64 clint_get_time(void) {
    u64 time;

    asm volatile("rdtime %0" : "=r"(time));

    return time;
}

static void clint_set_mtimecmp(u32 hart, s64 val) {
    if (hart >= MAX_HARTS) { return; }

    ((s64*)(CLINT_BASE + CLINT_MTIMECMP_OFFSET))[hart] = val;
}

static void clint_add_mtimecmp(u32 hart, s64 val) {
    clint_set_mtimecmp(hart, clint_get_time() + val);
}

u64 time_now(void) { return clint_get_time(); }

void timer_rel(u32 which_hart, s64 when) {
    clint_add_mtimecmp(which_hart, when);
}

void timer_abs(u32 which_hart, s64 when) {
    clint_set_mtimecmp(which_hart, when);
}

void timer_off(void) {
    u32 hartid;

    CSR_READ(hartid, "mhartid");

    clint_set_mtimecmp(hartid, CLINT_MTIMECMP_INFINITE);
}

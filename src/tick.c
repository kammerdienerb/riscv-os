#include "tick.h"
#include "sbi.h"
#include "sched.h"

static void set_next_tick(u32 hart) {
    sbicall(SBI_TIMER_REL, hart, DEFAULT_TICK);
}

void init_tick(void) {
    u32 hart;

    for (hart = 0; hart < MAX_HARTS; hart += 1) {
        set_next_tick(hart);
    }
}

void force_tick(u32 hart) {
    sbicall(SBI_TIMER_REL, hart, 0);
}

s64 do_tick(u32 hartid) {
    set_next_tick(hartid);
    sched_tick(hartid);
    return 0;
}

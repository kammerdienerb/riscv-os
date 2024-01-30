#ifndef __SBI_H__
#define __SBI_H__

#include "common.h"


#define LIST_SBICALL(X)                                                       \
    X(SBI_HART_ID,     "Get the ID of the current hart")                      \
    X(SBI_HART_STATUS, "Get the status of a hart")                            \
    X(SBI_START_HART,  "Start a hart at a given address")                     \
    X(SBI_STOP_HART,   "Stop the current hart")                               \
    X(SBI_PUTC,        "UART character print")                                \
    X(SBI_GETC,        "UART character receive")                              \
    X(SBI_CLOCK,       "Get the current clock value as reported by rdtime")   \
    X(SBI_TIMER_REL,   "Set a timer to trigger relative to now")              \
    X(SBI_TIMER_ABS,   "Set a timer to trigger at an absolute time")          \
    X(SBI_TIMER_CLEAR, "Stop sending an interrupt to the kernel for a timer")

#define SBICALL_ENUM(s, d) s,
enum {
    LIST_SBICALL(SBICALL_ENUM)
    NUM_SBICALL
};
#undef SBICALL_ENUM

__attribute__((noipa))
static inline s64 sbicall(u32 call, ...) {
    s64 ret;

    (void)call;

    asm volatile ("ecall");
    asm volatile("mv %0, a0" : "=r"(ret));

    return ret;
}

#endif

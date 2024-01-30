#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "common.h"


#define LIST_SYSCALL(X)                                                                 \
    X(SYS_EXIT,             "Exit the current process")                                 \
    X(SYS_GET_PID,          "The PID of the calling process")                           \
    X(SYS_SLEEP,            "Sleep the current process for some number of sched ticks") \
    X(SYS_PUTC,             "UART character print")                                     \
    X(SYS_GETC,             "UART character receive")                                   \
    X(SYS_RAND,             "Get 8 random bytes")                                       \
    X(SYS_INPUT_POLL,       "Wait for input event(s)")                                  \
    X(SYS_INPUT_POP,        "Pop an input event")                                       \
    X(SYS_GPU_GET_CTX,      "Acquire a GPU context")                                    \
    X(SYS_GPU_REL_CTX,      "Release a GPU context")                                    \
    X(SYS_GPU_CTX_PIXELS,   "Draw pixel data in a GPU context")                         \
    X(SYS_GPU_CTX_RECT,     "Draw a rectangle in a GPU context")                        \
    X(SYS_GPU_CTX_GET_RECT, "Get the rectangle of a GPU context")                       \
    X(SYS_GPU_COMMIT,       "Commit GPU updates")                                       \
    X(SYS_GPU_CLEAR,        "Clear window to a color")                                  \
    X(SYS_FILE_SIZE,        "Get the size of a file")                                   \
    X(SYS_FILE_READ,        "Read bytes from a file")                                   \
    X(SYS_MAP_MEM,          "Map memory into the process")

#define SYSCALL_ENUM(s, d) s,
enum {
    LIST_SYSCALL(SYSCALL_ENUM)
    NUM_SYSCALL
};
#undef SYSCALL_ENUM

__attribute__((noipa))
static inline s64 syscall(u32 call, ...) {
    s64 ret;

    (void)call;

    asm volatile("ecall");
    asm volatile("mv %0, a0" : "=r"(ret));

    return ret;
}

#endif

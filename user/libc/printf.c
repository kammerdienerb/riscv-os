#define KPRINT_NO_LOCK
#include "../../src/kprint.c"
#include "syscall.h"

#include <stdarg.h>

static void _putc(u8 c) { syscall(SYS_PUTC, c); }

void init_printf(void) {
    kprint_set_putc(_putc);
}

void printf(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vkprint(fmt, args);
    va_end(args);
}

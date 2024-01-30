#include "irq.h"
#include "kprint.h"
#include "uart.h"
#include "trap.h"
#include "sbicall.h"
#include "hart_mng.h"
#include "timer.h"

irq_handler_fn_t irq_handlers[NUM_IRQ];

#define IRQ_STRING(i, d, a, s) [IRQ_ENC(i, a)] = d,
const char *irq_descriptions[NUM_IRQ] = {
    LIST_IRQ(IRQ_STRING)
};
#undef IRQ_STRING



s64 handle_IRQ_USWI(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_SSWI(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }

s64 handle_IRQ_MSWI(u32 hartid, u64 cause) {
    return hart_handle_msip(hartid);
}

s64 handle_IRQ_UTIM(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_STIM(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }

s64 handle_IRQ_MTIM(u32 hartid, u64 cause) {
    u64 sip;

    timer_off();

    CSR_READ(sip, "mip");
    CSR_WRITE("mip", sip | SIP_STIP);

    return 0;
}

s64 handle_IRQ_UEXT(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_SEXT(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_MEXT(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }

s64 handle_IRQ_UART(u32 hartid, u64 cause) {
    uart_handle_irq();
    return 0;
}

s64 handle_IRQ_RCLK(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_PCIA(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_PCIB(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_PCIC(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_PCID(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_IMIS(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_IFLT(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_IILL(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_BRPT(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_LMIS(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_LFLT(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_SMIS(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_SFLT(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_USYS(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }

s64 handle_IRQ_SSYS(u32 hartid, u64 cause) {
    s64 err;
    u64 mepc;

    err = do_sbicall(hartid, trap_frame[hartid].gpregs[XREG_A0]);

    CSR_READ(mepc, "mepc");
    CSR_WRITE("mepc", mepc + 4);

    return err;
}

s64 handle_IRQ_MSYS(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_IPFT(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_LPFT(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }
s64 handle_IRQ_SPFT(u32 hartid, u64 cause) { kprint("unimplemented\n"); return -1; }

s64 handle_IRQ_IUNK(u32 hartid, u64 cause) {
    kprint("%c[irq]%_ %rUnknown IRQ! hartid = %u, cause = %X%_\n", hartid, cause);
    for (;;) WAIT_FOR_INTERRUPT();
    return -1;
}

s64 handle_IRQ_EUNK(u32 hartid, u64 cause) {
    kprint("%c[irq]%_ %rUnknown IRQ! hartid = %u, cause = %X%_\n", hartid, cause);
    for (;;) WAIT_FOR_INTERRUPT();
    return -1;
}


void init_irq_handlers(void) {
    u32 i;

#define IRQ_HANDLE(i, d, a, s) irq_handlers[IRQ_ENC(i, a)] = handle_##s;
    LIST_IRQ(IRQ_HANDLE);
#undef IRQ_HANDLE

    for (i = 0; i < NUM_IRQ; i += 1) {
        if (irq_handlers[i] == NULL) {
            irq_handlers[i] = handle_IRQ_IUNK;
        }
    }
}

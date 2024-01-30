#include "common.h"
#include "machine.h"
#include "uart.h"
#include "kprint.h"
#include "plic.h"
#include "trap.h"
#include "irq.h"
#include "hart.h"
#include "hart_mng.h"

void park_hart(void);
void trap_jump(void);

static u32 hart_0_is_ready = 0;

extern void * _bss_start;
extern void * _bss_end;
static void clear_bss(void) {
    u8 *p;
    p = _bss_start;
    while (p < (u8*)_bss_end) { *p = 0; p += 1; }
}

static void init_pmp(void) {
    CSR_WRITE("pmpaddr0", 0xffffffff >> 2);
    CSR_WRITE("pmpcfg0",  PMPCFG_AM(PMPAM_TOR) | PMPCFG_RD | PMPCFG_WR | PMPCFG_EX);
}

void main(u32 hartid) {
    if (hartid != 0) {
        while (!hart_0_is_ready);

        init_pmp();

        harts[hartid].status = HART_STOPPED;

        CSR_WRITE("mepc", park_hart);
        CSR_WRITE("mtvec", trap_jump);
        CSR_WRITE("mscratch", &(trap_frame[hartid].gpregs[0]));
        CSR_WRITE("sscratch", hartid);
        CSR_WRITE("mie", MIE_MSIE);
        CSR_WRITE("mideleg", 0);
        CSR_WRITE("medeleg", 0);
        CSR_WRITE("mstatus", MSTATUS_FS(1) | MSTATUS_MPP(MACHINE_MODE) | MSTATUS_MPIE);

        MRET();
    }

    clear_bss();

    init_uart();
    kprint_set_putc(uart_putc);
    kprint_set_getc(uart_getc);

    init_plic();
    init_irq_handlers();

    kprint("%mSBI, open up!%_\n");

    init_pmp();

    CSR_WRITE("mscratch", &(trap_frame[0].gpregs[0]));

    CSR_WRITE("mepc", 0x80050000ULL);

    CSR_WRITE("mie",   MIE_MEIE | MIE_MTIE | MIE_MSIE);

    /* Delegate software interrupts, timers,
       and _supervisor_ external interrupts to supervisor mode. */
    CSR_WRITE("mideleg",   (1 << INT_SSWI)
                         | (1 << INT_STIM)
                         | (1 << INT_SEXT));

    /* Delegate everything but reserved exceptions and ecalls
       from supervisor or machine mode to supervisor. */
    CSR_WRITE("medeleg",   EXC_BITS_ALL
                         & ~(1 << EXC___14)
                         & ~(1 << EXC_MSYS)
                         & ~(1 << EXC___10)
                         & ~(1 << EXC_SSYS));

    /* Enable the FPU and switch to supervisor mode with interrupts enabled. */
    CSR_WRITE("mstatus", MSTATUS_FS(1) | MSTATUS_MPP(SUPERVISOR_MODE) | MSTATUS_MPIE);

    harts[0].status = HART_STARTED;

    hart_0_is_ready = 1;

    MRET();
}

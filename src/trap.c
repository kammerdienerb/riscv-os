#include "common.h"
#include "trap.h"
#include "irq.h"
#include "plic.h"
#include "kprint.h"
#include "sbi.h"
#include "process.h"
#include "sched.h"

Trap_Frame trap_frame[MAX_HARTS];

void trap_handler(void) {
    u64         sscratch;
    u64         scause;
    u64         stval;
    u64         sepc;
    u64         hartid;
    u64         irq;
    u64         plic_irq;
    s64         err;

    hartid = sbicall(SBI_HART_ID);

    CSR_READ(sscratch, "sscratch");
    CSR_READ(scause,   "scause");
    CSR_READ(stval,    "stval");
    CSR_READ(sepc,     "sepc");

    irq      = IRQ_ENC(CAUSE_INTERRUPT(scause), CAUSE_ASYNC(scause));
    plic_irq = 0;

    if (irq >= NUM_IRQ) {
        irq = CAUSE_ASYNC(scause) ? IRQ_IUNK : IRQ_EUNK;
    } else if (irq == IRQ_SUPERVISOR_PLIC) {
        plic_irq = plic_claim(hartid, PLIC_MODE_SUPERVISOR);

        switch (plic_irq) {
            case PLIC_INT_UART:     irq = IRQ_UART; break;
            case PLIC_INT_RT_CLOCK: irq = IRQ_RCLK; break;
            case PLIC_INT_PCIEA:    irq = IRQ_PCIA; break;
            case PLIC_INT_PCIEB:    irq = IRQ_PCIB; break;
            case PLIC_INT_PCIEC:    irq = IRQ_PCIC; break;
            case PLIC_INT_PCIED:    irq = IRQ_PCID; break;
            default:
                irq      = IRQ_IUNK;
                plic_irq = 0;
                break;
        }
    }

    err = irq_handlers[irq](hartid, scause);

    if (err < 0) {
        kprint("%c[irq]%_ %rfailed to handle irq %U: %s%_\n", irq, irq_descriptions[irq]);
        kprint("  mode:   SUPERVISOR\n");
        kprint("  hartid: %u\n", hartid);
        kprint("  err:    %D\n", err);
        kprint("  sepc:   0x%X\n", sepc);
        kprint("  stval:  0x%X\n", stval);
        for (;;) { WAIT_FOR_INTERRUPT(); }
    }

    if (plic_irq && !err) {
        plic_complete(hartid, plic_irq, PLIC_MODE_SUPERVISOR);
    }
}

#include "common.h"
#include "trap.h"
#include "irq.h"
#include "plic.h"
#include "kprint.h"

Trap_Frame trap_frame[8];

void trap_handler(void) {
    u64 mcause;
    u64 mhartid;
    u64 mtval;
    u64 mepc;
    u64 irq;
    u64 plic_irq;
    s64 err;

    CSR_READ(mcause,  "mcause");
    CSR_READ(mhartid, "mhartid");
    CSR_READ(mtval,   "mtval");
    CSR_READ(mepc,    "mepc");

    irq      = IRQ_ENC(CAUSE_INTERRUPT(mcause), CAUSE_ASYNC(mcause));
    plic_irq = 0;

    if (irq >= NUM_IRQ) {
        irq = CAUSE_ASYNC(mcause) ? IRQ_IUNK : IRQ_EUNK;
    } else if (irq == IRQ_MACHINE_PLIC) {
        plic_irq = plic_claim(mhartid, PLIC_MODE_MACHINE);

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

    err = irq_handlers[irq](mhartid, mcause);

    if (err < 0) {
        kprint("%c[irq]%_ %rfailed to handle irq %U: %s%_\n", irq, irq_descriptions[irq]);
        kprint("  mode:   MACHINE\n");
        kprint("  hartid: %u\n", mhartid);
        kprint("  err:    %D\n", err);
        kprint("  mepc:   0x%X\n", mepc);
        kprint("  mtval:  0x%X\n", mtval);
        for (;;) { WAIT_FOR_INTERRUPT(); }
    }

    if (plic_irq && !err) {
        plic_complete(mhartid, plic_irq, PLIC_MODE_MACHINE);
    }
}

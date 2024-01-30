#include "plic.h"
#include "common.h"
#include "machine.h"

void init_plic(void) {
    plic_enable(0, PLIC_INT_UART, PLIC_MODE_MACHINE);
    plic_set_priority(PLIC_INT_UART, 7);

    plic_enable(0, PLIC_INT_PCIEA, PLIC_MODE_SUPERVISOR);
    plic_set_priority(PLIC_INT_PCIEA, 7);

    plic_enable(0, PLIC_INT_PCIEB, PLIC_MODE_SUPERVISOR);
    plic_set_priority(PLIC_INT_PCIEB, 7);

    plic_enable(0, PLIC_INT_PCIEC, PLIC_MODE_SUPERVISOR);
    plic_set_priority(PLIC_INT_PCIEC, 7);

    plic_enable(0, PLIC_INT_PCIED, PLIC_MODE_SUPERVISOR);
    plic_set_priority(PLIC_INT_PCIED, 7);

    plic_set_threshold(0, 0, PLIC_MODE_MACHINE);
    plic_set_threshold(0, 0, PLIC_MODE_SUPERVISOR);
}

void plic_set_priority(u32 interrupt_id, u8 priority) {
    volatile u32 *base;

    base = (u32*)PLIC_PRIORITY(interrupt_id);

    *base = priority & 0x7;
}

void plic_set_threshold(u32 hartid, u8 priority, u32 mode) {
    volatile u32 *base;

    base = (u32*)PLIC_THRESHOLD(hartid, mode);

    *base = priority & 0x7;
}

void plic_enable(u32 hartid, u32 interrupt_id, u32 mode) {
    volatile u32 *base;

    base = (u32*)PLIC_ENABLE(hartid, mode);

    base[interrupt_id >> 5] |= 1UL << (interrupt_id & 0x1f);
}

void plic_disable(u32 hartid, u32 interrupt_id, u32 mode) {
    volatile u32 *base;

    base = (u32*)PLIC_ENABLE(hartid, mode);

    base[interrupt_id >> 5] &= ~(1UL << (interrupt_id & 0x1f));
}

u32 plic_claim(u32 hartid, u32 mode) {
    volatile u32 *base;

    base = (u32*)PLIC_CLAIM(hartid, mode);

    return *base;
}

void plic_complete(u32 hartid, u32 id, u32 mode) {
    volatile u32 *base;

    base = (u32*)PLIC_CLAIM(hartid, mode);

    *base = id;
}

#ifndef __IRQ_H__
#define __IRQ_H__

#include "common.h"
#include "machine.h"

#define LIST_IRQ(X)                                                              \
/*  Volume II: RISC-V Privileged Architectures V20190608-Priv-MSU-Ratified 37    \
    -------------------------------------------------------------------------    \
      Interrupt  Description                          Async  Symbol              \
    ------------------------------------------------------------------------- */ \
    X(INT_USWI,  "User software interrupt",           1,     IRQ_USWI)           \
    X(INT_SSWI,  "Supervisor software interrupt",     1,     IRQ_SSWI)           \
    X(INT_MSWI,  "Machine software interrupt",        1,     IRQ_MSWI)           \
    X(INT_UTIM,  "User timer interrupt",              1,     IRQ_UTIM)           \
    X(INT_STIM,  "Supervisor timer interrupt",        1,     IRQ_STIM)           \
    X(INT_MTIM,  "Machine timer interrupt",           1,     IRQ_MTIM)           \
    X(INT_UEXT,  "User external interrupt",           1,     IRQ_UEXT)           \
    X(INT_SEXT,  "Supervisor external interrupt",     1,     IRQ_SEXT)           \
    X(INT_MEXT,  "Machine external interrupt",        1,     IRQ_MEXT)           \
    X(INT_UART,  "PLIC: UART0",                       1,     IRQ_UART)           \
    X(INT_RCLK,  "PLIC: Realtime Clock",              1,     IRQ_RCLK)           \
    X(INT_PCIA,  "PLIC: PCIE A",                      1,     IRQ_PCIA)           \
    X(INT_PCIB,  "PLIC: PCIE B",                      1,     IRQ_PCIB)           \
    X(INT_PCIC,  "PLIC: PCIE C",                      1,     IRQ_PCIC)           \
    X(INT_PCID,  "PLIC: PCIE D",                      1,     IRQ_PCID)           \
    X(INT_UNKN,  "Unknown interrupt",                 1,     IRQ_IUNK)           \
    X(EXC_IMIS,  "Instruction address misaligned",    0,     IRQ_IMIS)           \
    X(EXC_IFLT,  "Instruction access fault",          0,     IRQ_IFLT)           \
    X(EXC_IILL,  "Illegal instruction",               0,     IRQ_IILL)           \
    X(EXC_BRPT,  "Breakpoint",                        0,     IRQ_BRPT)           \
    X(EXC_LMIS,  "Load address misaligned",           0,     IRQ_LMIS)           \
    X(EXC_LFLT,  "Load access fault",                 0,     IRQ_LFLT)           \
    X(EXC_SMIS,  "Store/AMO address misaligned",      0,     IRQ_SMIS)           \
    X(EXC_SFLT,  "Store/AMO access fault",            0,     IRQ_SFLT)           \
    X(EXC_USYS,  "Environment call from U-mode",      0,     IRQ_USYS)           \
    X(EXC_SSYS,  "Environment call from S-mode",      0,     IRQ_SSYS)           \
    X(EXC_MSYS,  "Environment call from M-mode",      0,     IRQ_MSYS)           \
    X(EXC_IPFT,  "Instruction page fault",            0,     IRQ_IPFT)           \
    X(EXC_LPFT,  "Load page fault",                   0,     IRQ_LPFT)           \
    X(EXC_SPFT,  "Store/AMO page fault",              0,     IRQ_SPFT)           \
    X(EXC_UNKN,  "Unknown exception",                 0,     IRQ_EUNK)

#define IRQ_ENC(i, a) (((i) << 1) | (a))

#define IRQ_ENUM(i, d, a, s) s = IRQ_ENC(i, a),
enum {
    LIST_IRQ(IRQ_ENUM)
    NUM_IRQ
};
#undef IRQ_ENUM

#define IRQ_MACHINE_PLIC    (IRQ_MEXT)
#define IRQ_SUPERVISOR_PLIC (IRQ_SEXT)


typedef s64 (*irq_handler_fn_t)(u32, u64);

extern irq_handler_fn_t irq_handlers[NUM_IRQ];
extern const char *irq_descriptions[NUM_IRQ];


void init_irq_handlers(void);


#endif

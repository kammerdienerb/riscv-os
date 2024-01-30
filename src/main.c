#include "common.h"
#include "machine.h"
#include "sbi.h"
#include "kprint.h"
#include "trap.h"
#include "irq.h"
#include "hart.h"
#include "symbols.h"
#include "page.h"
#include "mmu.h"
#include "kmalloc.h"
#include "array.h"
#include "driver.h"
#include "pci.h"
#include "gpu.h"
#include "elf.h"
#include "syscall.h"
#include "process.h"
#include "tick.h"
#include "sched.h"
#include "vfs.h"
#include "kconsole.h"

static void sbi_putc(u8 c) { sbicall(SBI_PUTC, c);     }
static u8   sbi_getc(void) { return sbicall(SBI_GETC); }

void trap_jump(void);

extern void * _bss_start;
extern void * _bss_end;
static void clear_bss(void) {
    u8 *p;
    p = _bss_start;
    while (p < (u8*)_bss_end) { *p = 0; p += 1; }
}


__attribute__((noreturn))
void main(u32 hartid) {
    clear_bss();

    kprint_set_putc(sbi_putc);
    kprint_set_getc(sbi_getc);

    kprint("%bNew boot goofin'.%_\n");

    init_irq_handlers();

    trap_frame[0].hartid = 0;

    CSR_WRITE("stvec",    trap_jump);
    CSR_WRITE("sscratch", &(trap_frame[0].gpregs[0]));
    CSR_WRITE("sie",      SIE_SEIE | SIE_STIE);

    init_page_allocator();

    init_mmu();
    activate_mmu();
    init_kmalloc();
    init_drivers();
    init_pci();
    init_vfs();

    init_sched();
    init_tick();

    gpu_clear(0xFF7f7f7f);
    gpu_commit();

    start_kconsole();

    for (;;) { WAIT_FOR_INTERRUPT(); }
}

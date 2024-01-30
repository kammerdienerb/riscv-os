.section .text.init
.global _start
_start:
    csrr  a0, mhartid
.option push
.option norelax
    la    sp, _stack_end
    la    gp, __global_pointer$
.option pop
    # Now that we have multiple HARTs, we need multiple stacks
    csrr    t0, mhartid
    slli    t0, t0, 12
    sub     sp, sp, t0

    la    t1, main
    csrw  mepc, t1
    li    t1, (3 << 11) | (1 << 7) # MPP machine mode, MPIE=yes
    csrw  mstatus, t1
    li    t1, (1 << 11) # MEIE=yes, enable external interrupts
    csrw  mie, t1
    la    t1, trap_jump
    csrw  mtvec, t1
    mret

.global park_hart
park_hart:
    wfi
    j park_hart

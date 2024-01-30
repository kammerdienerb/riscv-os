.section .text.init
.global _start
_start:
.option push
.option norelax
    la    sp, _stack_end
    la    gp, __global_pointer$
.option pop
    la    t0,      main
    csrw  sepc,    t0
    li    t0,      (1 << 13) | (1 << 8) | (1 << 5)
    csrw  sstatus, t0
    sret

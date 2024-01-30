.section .text
.global _start
.align 4
_start:
    la      gp, __global_pointer$

    la      t0, _bss_start
    la      t1, _bss_end
1:
    bge     t0, t1, 1f
    sd      zero, (t0)
    addi    t0, t0, 8
    j       1b
1:

    call    libc_init
    call    main

    li      a0, 0  # exit
    ecall

park:
    j park

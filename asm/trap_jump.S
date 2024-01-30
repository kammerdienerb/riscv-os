.section .text
.altmacro

#define GP_BASE 0
#define FP_BASE (32*8)

.macro savegp i, m=x31
    sd    x\i, (GP_BASE + 8 * \i)(\m)
.endm

.macro loadgp i, m=x31
    ld    x\i, (GP_BASE + 8 * \i)(m)
.endm

.macro savefp i, m=x31
    fsd f\i, (FP_BASE + 8 * \i)(m)
.endm

.macro loadfp i, m=x31
    fld f\i, (FP_BASE + 8 * \i)(m)
.endm


.global trap_jump
.align 4
trap_jump:
    csrrw    t6, sscratch, t6

    .set i, 1
    .rept 30
        savegp %i
        .set i, i+1
    .endr

    mv        t5, t6
    csrrw    t6, sscratch, t6
    savegp    31, t5

    csrr    t0, sstatus
    srli    t0, t0, 13
    andi    t0, t0, 3
    beqz    t0, 1f
    .set i, 0
    .rept 32
        savefp %i, t5
        .set i, i+1
    .endr
1:

    # Now that we have multiple HARTs, we need multiple stacks
    la      sp, _trap_stack_end
    li      a0, 0 # SBI_HART_ID
    ecall
    slli    a0, a0, 12
    sub     sp, sp, a0

    call    trap_handler

    csrr    t6, sscratch

    csrr    t0, sstatus
    srli    t0, t0, 13
    andi    t0, t0, 3
    li      t1, 2
    blt     t0, t1, 1f


    .set i, 0
    .rept 32
        loadfp %i
        .set i, i+1
    .endr

1:
    .set i, 1
    .rept 31
        loadgp %i
        .set i, i+1
    .endr


    sret

.section .text
.altmacro

.macro savegp i
    sd  x\i, (8 * \i)(t6)
.endm

.macro loadgp i
    ld  x\i, (8 * \i)(t6)
.endm

.align 4
.global trap_jump
trap_jump:
    csrrw   t6, mscratch, t6

    # Save context
    .set i, 1
    .rept 30
        savegp %i
        .set i, i + 1
    .endr

    mv      t5, t6
    csrrw   t6, mscratch, t6
    sd      t6, 8*31(t5)

.option push
.option norelax
    la      sp, _stack_end
    la      gp, __global_pointer$
.option pop
    # Now that we have multiple HARTs, we need multiple stacks
    csrr    t0, mhartid
    slli    t0, t0, 12
    sub     sp, sp, t0

    call    trap_handler
    csrr    t6, mscratch

    .set i, 1
    .rept 31
        loadgp %i
        .set i, i + 1
    .endr
    # Load context

    mret

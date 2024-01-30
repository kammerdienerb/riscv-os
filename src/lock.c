#include "lock.h"
#include "kmalloc.h"

s32 sem_trydown(Sem *sem) {
    s32 old;

    asm volatile ("amoadd.w %0, %1, (%2)"
                  : "=r"(old)
                  : "r"(-1), "r"(&sem->s));

    if (old <= 0) {
        /* We don't have the resource, but we decremented it, so we may have -1
           -2, or so on. So, let's undo what we did to get back to 0. */
        sem_up(sem);
    }

    return old > 0;
}

void sem_up(Sem *sem) {
    asm volatile ("amoadd.w zero, %0, (%1)"
                  :
                  : "r"(1), "r"(&sem->s));
}

int spin_trylock(Spinlock *spin) {
    s32 old;

    asm volatile("amoswap.w.aq %0, %1, (%2)"
                  : "=r"(old)
                  : "r"(SPIN_LOCKED), "r"(&spin->s));

    return old != SPIN_LOCKED;
}

void spin_lock(Spinlock *spin) {
    while (!spin_trylock(spin));
}

void spin_unlock(Spinlock *spin) {
    asm volatile("amoswap.w.rl zero, zero, (%0)"
                 : : "r"(&spin->s));
}

#if 0
void barrier_init(Barrier *barrier) {
    barrier->head  = NULL;
    barrier->value = 0;
}

void barrier_add(Barrier *barrier, void *v) {
    Barrier_List *list;

    bl              = kmalloc(sizeof(*bl));
    bl->v           = v;
    bl->next        = barrier->head;
    barrier->head   = bl;
    barrier->value += 1;
}

void barrier_at(Barrier *barrier) {
    s32           old;
    Barrier_List *bl;
    Barrier_List *next;

    asm volatile("amoadd.w %0, %1, (%2)"
                 : "=r"(old)
                 : "r"(-1), "r"(&barrier->value));

    if (old <= 1) {
        for (bl = barrier->head; NULL != bl; bl = next) {
            next = bl->next;
            bl->proc->state = STATE_RUNNING;
            kfree(bl);
        }
        barrier->head = NULL;
    }
}
#endif

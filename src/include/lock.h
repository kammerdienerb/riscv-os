#ifndef __LOCK_H__
#define __LOCK_H__

#include "common.h"

typedef struct {
    s32 s;
} Sem;

s32  sem_trydown(Sem *sem);
void sem_up(Sem *sem);

#define SPIN_LOCKED   (1)
#define SPIN_UNLOCKED (0)

typedef struct {
    s32 s;
} Spinlock;

int  spin_trylock(Spinlock *spin);
void spin_lock(Spinlock *spin);
void spin_unlock(Spinlock *spin);

#if 0
typedef struct Barrier_List {
    void                *v;
    struct Barrier_List *next;
} Barrier_List;

typedef struct {
    Barrier_List *head;
    s32           value;
} Barrier;
#endif

#endif

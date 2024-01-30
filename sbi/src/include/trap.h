#ifndef __TRAP_H__
#define __TRAP_H__

#include "machine.h"

typedef struct {
    u64 gpregs[32];
    u64 fpregs[32];
    u64 pc;
} Trap_Frame;

extern Trap_Frame trap_frame[MAX_HARTS];

#endif

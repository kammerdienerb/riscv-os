#ifndef __TICK_H__
#define __TICK_H__

#include "common.h"

#define DEFAULT_TICK (100000)

void init_tick(void);
void force_tick(u32 hart);
s64 do_tick(u32 hart);

#endif

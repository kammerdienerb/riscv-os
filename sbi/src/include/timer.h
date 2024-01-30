#ifndef __TIMER_H__
#define __TIMER_H__

#include "common.h"

u64  time_now(void);
void timer_rel(u32 which_hart, s64 when);
void timer_abs(u32 which_hart, s64 when);
void timer_off(void);

#endif

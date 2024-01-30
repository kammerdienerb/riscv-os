#ifndef __PLIC_H__
#define __PLIC_H__

#include "common.h"

void init_plic(void);
void plic_set_priority(u32 interrupt_id, u8 priority);
void plic_set_threshold(u32 hartid, u8 priority, u32 mode);
void plic_enable(u32 hartid, u32 interrupt_id, u32 mode);
void plic_disable(u32 hartid, u32 interrupt_id, u32 mode);
u32  plic_claim(u32 hartid, u32 mode);
void plic_complete(u32 hartid, u32 id, u32 mode);

#endif

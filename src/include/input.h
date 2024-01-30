#ifndef __INPUT_H__
#define __INPUT_H__

#include "common.h"
#include "input_common.h"

void input_push(Input_Event *event);
s64  input_pull(Input_Event *event);
u32  input_ready(void);

#endif

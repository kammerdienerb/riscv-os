#ifndef __INPUT_COMMON_H__
#define __INPUT_COMMON_H__

#include "input_event_codes.h"

#define EV_DISP (0xff)

typedef struct {
    u16 type;
    u16 code;
    u32 value;
} Input_Event;

#endif

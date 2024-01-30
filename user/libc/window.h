#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "common.h"
#include "../../src/include/input_event_codes.h"

#define RGBA(_r, _g, _b, _a) ((_r) | ((_g) << 8ULL) | ((_b) << 16ULL) | ((_a) << 24ULL))

enum {
    WIN_EVT_NONE,
    WIN_EVT_KEY,
    WIN_EVT_CURSOR,
};

typedef struct {
    u32 kind;
    union {
        struct {
            u32 key;
            u32 action;
        };
        struct {
            u32 x;
            u32 y;
        };
    };
} Win_Event;

s64  win_ctx(void);
void win_ctx_release(s64 ctx);
void win_clear(s64 ctx, u32 rgba_color);
void win_get_rect(s64 ctx, u32 *x, u32 *y, u32 *w, u32 *h);
void win_pixels(s64 ctx, u32 x, u32 y, u32 w, u32 h, u32 *pixels);
void win_rect(s64 ctx, u32 x, u32 y, u32 w, u32 h, u32 rgba_color);
void win_commit(s64 ctx);
void win_poll_events(s64 ctx);
u32  win_event(s64 ctx, Win_Event *event);

#endif

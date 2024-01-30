#include "window.h"
#include "syscall.h"
#include "../../src/include/input_common.h"

typedef struct {
    u32 x;
    u32 y;
    u32 w;
    u32 h;
} Win;

static Win wins[2];

static void update_win(s64 ctx) {
    syscall(SYS_GPU_CTX_GET_RECT,
            ctx,
            &wins[ctx].x,
            &wins[ctx].y,
            &wins[ctx].w,
            &wins[ctx].h);
}

s64 win_ctx(void) {
    s64 ctx;
    ctx = syscall(SYS_GPU_GET_CTX);

    if (ctx >= 0) {
        update_win(ctx);
    }

    return ctx;
}

void win_ctx_release(s64 ctx) {
    syscall(SYS_GPU_REL_CTX, ctx);
}

void win_clear(s64 ctx, u32 rgba_color) {
    syscall(SYS_GPU_CLEAR, ctx, rgba_color);
}

void win_get_rect(s64 ctx, u32 *x, u32 *y, u32 *w, u32 *h) {
    *x = wins[ctx].x;
    *y = wins[ctx].y;
    *w = wins[ctx].w;
    *h = wins[ctx].h;
}

void win_pixels(s64 ctx, u32 x, u32 y, u32 w, u32 h, u32 *pixels) {
    syscall(SYS_GPU_CTX_PIXELS, ctx, x, y, w, h, pixels);
}

void win_rect(s64 ctx, u32 x, u32 y, u32 w, u32 h, u32 rgba_color) {
    syscall(SYS_GPU_CTX_RECT, ctx, x, y, w, h, rgba_color);
}

void win_commit(s64 ctx) {
    syscall(SYS_GPU_COMMIT, ctx);
}

void win_poll_events(s64 ctx) {
    (void)ctx;
    syscall(SYS_INPUT_POLL);
}

static u32 save_x;
static u32 save_y;

u32 win_event(s64 ctx, Win_Event *event) {
    s64         status;
    Input_Event e;

    (void)ctx;

    event->kind = WIN_EVT_NONE;

    status = syscall(SYS_INPUT_POP, &e);

    if (status == 0) {
        switch (e.type) {
            case EV_DISP:
                update_win(ctx);
                break;
            case EV_KEY:
                event->kind   = WIN_EVT_KEY;
                event->key    = e.code;
                event->action = e.value;
                break;
            case EV_ABS:
                if (e.code == ABS_X) {
                    save_x = (((u64)e.value) * (2 * (u64)wins[ctx].w)) / 32767ULL;
                } else if (e.code == ABS_Y) {
                    save_y = (((u64)e.value) * (u64)wins[ctx].h) / 32767ULL;
                }
                event->kind = WIN_EVT_CURSOR;
                event->x    = save_x;
                event->y    = save_y;
                break;
        }
        return 1;
    }

    return 0;
}

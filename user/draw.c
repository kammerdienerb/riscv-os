#include "syscall.h"
#include "printf.h"
#include "window.h"

enum {
    NO_MODE,
    DRAW_MODE,
    ERASE_MODE,
};

s64 x;
s64 y;
u32 mode;
u32 color;

#define BG (RGBA(255, 255, 255, 255))

void main(void) {
    s64       ctx;
    Win_Event event;

    ctx = win_ctx();

    if (ctx == -1) {
        printf("[draw]: %rFailed to get window context!%_\n");
        return;
    }

    while (win_event(ctx, &event)) {} /* empty event queue */

    color = RGBA(0, 0, 255, 255);

    win_clear(ctx, BG);
    win_commit(ctx);

    for (;;) {
        win_poll_events(ctx);

        while (win_event(ctx, &event)) {
            switch (event.kind) {
                case WIN_EVT_KEY:
                    switch (event.key) {
                        case BTN_LEFT:
                            if (event.action == 1) {
                                mode = DRAW_MODE;
                            } else if (event.action == 0) {
                                mode = NO_MODE;
                            }
                            break;
                        case BTN_RIGHT:
                            if (event.action == 1) {
                                mode = ERASE_MODE;
                            } else if (event.action == 0) {
                                mode = NO_MODE;
                            }
                            break;
                        case KEY_R:
                            color = RGBA(255, 0, 0, 255);
                            break;
                        case KEY_G:
                            color = RGBA(0, 255, 0, 255);
                            break;
                        case KEY_B:
                            color = RGBA(0, 0, 255, 255);
                            break;
                        case KEY_Y:
                            color = RGBA(255, 255, 0, 255);
                            break;
                        case KEY_C:
                            color = RGBA(0, 255, 255, 255);
                            break;
                        case KEY_M:
                            color = RGBA(255, 0, 255, 255);
                            break;
                        case KEY_K:
                            color = RGBA(0, 0, 0, 255);
                            break;
                        case KEY_O:
                            color = RGBA(255, 128, 0, 255);
                            break;
                        case KEY_Q:
                            win_clear(ctx, RGBA(0, 0, 0, 255));
                            goto out_release;
                        }
                    break;
                case WIN_EVT_CURSOR:
                    x = event.x;
                    y = event.y;
                    break;
            }

            if (event.kind != WIN_EVT_NONE) {
                if (mode == DRAW_MODE) {
                    win_rect(ctx, x, y, 5, 5, color);
                } else if (mode == ERASE_MODE) {
                    win_rect(ctx, x, y, 10, 10, BG);
                }
            }
        }

        win_commit(ctx);
    }

out_release:;
    win_ctx_release(ctx);
}

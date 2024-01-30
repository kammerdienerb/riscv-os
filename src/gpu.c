#include "gpu.h"
#include "driver.h"
#include "kprint.h"

static Driver_State *find_gpu_driver(void) {
    Driver       *driver;
    Driver_State *state;

    if (first_driver_of_type(DRV_GPU, &driver, &state) != 0) {
        kprint("no viable GPU driver or device found\n");
        return NULL;
    }

    return state;
}

s64 gpu_reset_display(void) {
    Driver_State *state;

    if ((state = find_gpu_driver()) != NULL) {
        return state->driver->gpu.reset_display(state);
    }

    return -1;
}

s64 gpu_clear(u32 rgba_color) {
    Driver_State *state;

    if ((state = find_gpu_driver()) != NULL) {
        return state->driver->gpu.clear(state, rgba_color);
    }

    return -1;
}

s64 gpu_get_rect(u32 *x, u32 *y, u32 *w, u32 *h) {
    Driver_State *state;

    if ((state = find_gpu_driver()) != NULL) {
        return state->driver->gpu.get_rect(state, x, y, w, h);
    }

    return -1;
}

s64 gpu_pixels(u32 x, u32 y, u32 w, u32 h, u32 *pixels) {
    Driver_State *state;

    if ((state = find_gpu_driver()) != NULL) {
        return state->driver->gpu.pixels(state, x, y, w, h, pixels);
    }

    return -1;
}

s64 gpu_rect(u32 x, u32 y, u32 w, u32 h, u32 rgba_color) {
    Driver_State *state;

    if ((state = find_gpu_driver()) != NULL) {
        return state->driver->gpu.rect(state, x, y, w, h, rgba_color);
    }

    return -1;
}

s64 gpu_commit(void) {
    Driver_State *state;

    if ((state = find_gpu_driver()) != NULL) {
        return state->driver->gpu.commit(state);
    }

    return -1;
}

static u8 ctx_bits;

s64 gpu_ctx(void) {
    switch (ctx_bits) {
        case 0:
        case 2:
            ctx_bits |= 1 << 0;
            gpu_reset_display();
            return 0;
        case 1:
            ctx_bits |= 1 << 1;
/*             gpu_reset_display(); */
            return 1;
    }

    return -1;
}

void gpu_ctx_release(s64 ctx) {
    ctx_bits &= ~(1 << ctx);
    gpu_reset_display();
}

void gpu_ctx_commit(s64 ctx) {
    (void)ctx;
    gpu_commit();
}

void gpu_ctx_rect(s64 ctx, u32 x, u32 y, u32 w, u32 h, u32 rgba_color) {
    u32 cx;
    u32 cy;
    u32 cw;
    u32 ch;

    gpu_ctx_get_rect(ctx, &cx, &cy, &cw, &ch);

    x = MIN(cx + x, cx + cw);
    y = MIN(cy + y, cy + ch);
    w = MIN(w, (cx + cw) - x);
    h = MIN(h, (cx + ch) - y);

    gpu_rect(x, y, w, h, rgba_color);
}

void gpu_ctx_pixels(s64 ctx, u32 x, u32 y, u32 w, u32 h, u32 *pixels) {
    u32 cx;
    u32 cy;
    u32 cw;
    u32 ch;

    gpu_ctx_get_rect(ctx, &cx, &cy, &cw, &ch);

    x = MIN(cx + x, cx + cw);
    y = MIN(cy + y, cy + ch);
    w = MIN(w, (cx + cw) - x);
    h = MIN(h, (cx + ch) - y);

    gpu_pixels(x, y, w, h, pixels);
}

void gpu_ctx_get_rect(s64 ctx, u32 *x, u32 *y, u32 *w, u32 *h) {
    u32 gx;
    u32 gy;
    u32 gw;
    u32 gh;

    gpu_get_rect(&gx, &gy, &gw, &gh);

    *x = ctx * (gw / 2);
    *y = gy;
    *w = gw / 2;
    *h = gh;
}

void gpu_ctx_clear(s64 ctx, u32 rgba_color) {
    u32 x;
    u32 y;
    u32 w;
    u32 h;

    gpu_ctx_get_rect(ctx, &x, &y, &w, &h);

    gpu_rect(x, y, w, h, rgba_color);
}

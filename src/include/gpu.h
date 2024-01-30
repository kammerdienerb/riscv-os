#ifndef __GPU_H__
#define __GPU_H__

#include "common.h"

s64  gpu_reset_display(void);
s64  gpu_clear(u32 rgba_color);
s64  gpu_get_rect(u32 *x, u32 *y, u32 *w, u32 *h);
s64  gpu_pixels(u32 x, u32 y, u32 w, u32 h, u32 *pixels);
s64  gpu_rect(u32 x, u32 y, u32 w, u32 h, u32 rgba_color);
s64  gpu_commit(void);
s64  gpu_ctx(void);
void gpu_ctx_release(s64 ctx);
void gpu_ctx_rect(s64 ctx, u32 x, u32 y, u32 w, u32 h, u32 rgba_color);
void gpu_ctx_pixels(s64 ctx, u32 x, u32 y, u32 w, u32 h, u32 *pixels);
void gpu_ctx_get_rect(s64 ctx, u32 *x, u32 *y, u32 *w, u32 *h);
void gpu_ctx_commit(s64 ctx);
void gpu_ctx_clear(s64 ctx, u32 rgba_color);

#endif

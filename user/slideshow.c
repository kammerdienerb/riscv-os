#include "syscall.h"
#include "printf.h"
#include "window.h"

static inline s32 is_digit(s32 c) {
    return (u32)(('0' - 1 - c) & (c - ('9' + 1))) >> (sizeof(c) * 8 - 1);
}

static s32 stoi(const char *s) {
    u32 is_neg;
    s32 r;

    is_neg = *s == '-';
    if (is_neg) { s += 1; }

    r = 0;
    while (*s && is_digit(*s)) {
        r *= 10;
        r += *s - '0';
        s += 1;
    }

    if (is_neg) { r = -r; }

    return r;
}

void main(void) {
    s64   ctx;
    u8   *buff;
    u32  *image;
    s64   len;
    char *p;
    u64   i;
    u64   j;
    u32   x;
    u32   y;
    u32   w;
    u32   h;

    ctx = win_ctx();

    if (ctx == -1) {
        printf("[slideshow]: %rFailed to get window context!%_\n");
        return;
    }

    buff  = (void*)syscall(SYS_MAP_MEM, 8192);
    image = (void*)syscall(SYS_MAP_MEM, KB(64));

    len = syscall(SYS_FILE_SIZE, "/EYE.PGM");
    syscall(SYS_FILE_READ, "/EYE.PGM", buff, 0, len);

    p = (char*)buff;
    while (*p) { if (*p == '\n') { p += 1; break; } p += 1; }
    while (*p) { if (*p == '\n') { p += 1; break; } p += 1; }
    w = stoi(p);
    while (*p) { if (*p == ' ')  { p += 1; break; } p += 1; }
    h = stoi(p);
    while (*p) { if (*p == '\n') { p += 1; break; } p += 1; }
    while (*p) { if (*p == '\n') { p += 1; break; } p += 1; }

    printf("image is %u x %u\n", w, h);

    for (i = 0; i < w; i += 1) {
        for (j = 0; j < h; j += 1) {
            image[i * w + j] = RGBA(*p, *p, *p, 255);
            p += 1;
        }
    }

    for (i = 0; 1; i += 1) {
        win_get_rect(ctx, &x, &y, &w, &h);
        win_clear(ctx, RGBA(0, 255, 0, 255));
        win_pixels(ctx, i, 0, w, h, image);
        win_commit(ctx);
        syscall(SYS_SLEEP, 1000);
    }

out_release:;
    win_ctx_release(ctx);
}

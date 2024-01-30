#include "syscall.h"
#include "sbi.h"
#include "trap.h"
#include "process.h"
#include "kprint.h"
#include "rng.h"
#include "sched.h"
#include "gpu.h"
#include "input.h"
#include "mmu.h"
#include "vfs.h"
#include "page.h"

s64 handle_SYS_EXIT(s64 exit_code) {
    sched_exit_current(exit_code);
    return 0;
}

s64 handle_SYS_PUTC(u8 c) {
    return sbicall(SBI_PUTC, c);
}

s64 handle_SYS_GETC(void) {
    return sbicall(SBI_GETC);
}

s64 handle_SYS_RAND(void) {
    u64            sscratch;
    Process_Frame *frame;
    s64            status;
    u64            random_bytes;

    CSR_READ(sscratch, "sscratch");
    frame = (void*)sscratch;

    status = rng_fill((u8*)(void*)&random_bytes, sizeof(random_bytes));

    if (status == 0) {
        frame->gpregs[XREG_A0] = random_bytes;
    }

    return status;
}

s64 handle_SYS_GET_PID(void) {
    u64            sscratch;
    Process_Frame *frame;
    Process       *current;

    CSR_READ(sscratch, "sscratch");
    frame = (void*)sscratch;

    current = sched_current(sbicall(SBI_HART_ID));

    if (current == NULL) { return -1; }

    frame->gpregs[XREG_A0] = current->pid;

    return 0;
}

s64 handle_SYS_SLEEP(u64 n_ticks) {
    sched_sleep_current(n_ticks);
    return 0;
}

s64 handle_SYS_INPUT_POLL(void) {
    if (input_ready()) { return 0; }

    sched_wait_current(PROC_WAIT_INPUT);

    return 0;
}

s64 handle_SYS_INPUT_POP(void *user_event) {
    u64            sscratch;
    Process_Frame *frame;
    Input_Event    event;

    CSR_READ(sscratch, "sscratch");
    frame = (void*)sscratch;

    if (input_pull(&event)) {
        frame->gpregs[XREG_A0] = 0;
        kernel_to_user(user_event, &event, sizeof(event));
    } else {
        frame->gpregs[XREG_A0] = -1;
    }

    return 0;
}

s64 handle_SYS_GPU_GET_CTX(void) {
    u64            sscratch;
    Process_Frame *frame;

    CSR_READ(sscratch, "sscratch");
    frame = (void*)sscratch;

    frame->gpregs[XREG_A0] = gpu_ctx();

    return 0;
}

s64 handle_SYS_GPU_REL_CTX(s64 ctx) {
    gpu_ctx_release(ctx);
    return 0;
}

s64 handle_SYS_GPU_CTX_PIXELS(s64 ctx, u32 x, u32 y, u32 w, u32 h, u32 *upixels) {
    u32 *pixels;

    pixels = kmalloc(w * h * sizeof(u32));

    user_to_kernel(pixels, upixels, w * h * sizeof(u32));

    gpu_ctx_pixels(ctx, x, y, w, h, pixels);

    kfree(pixels);

    return 0;
}

s64 handle_SYS_GPU_CTX_RECT(s64 ctx, u32 x, u32 y, u32 w, u32 h, u32 rgba_color) {
    gpu_ctx_rect(ctx, x, y, w, h, rgba_color);

    return 0;
}

s64 handle_SYS_GPU_CTX_GET_RECT(s64 ctx, u32 *ux, u32 *uy, u32 *uw, u32 *uh) {
    u32 x;
    u32 y;
    u32 w;
    u32 h;

    gpu_ctx_get_rect(ctx, &x, &y, &w, &h);

    kernel_to_user(ux, &x, sizeof(*ux));
    kernel_to_user(uy, &y, sizeof(*uy));
    kernel_to_user(uw, &w, sizeof(*uw));
    kernel_to_user(uh, &h, sizeof(*uh));

    return 0;
}

s64 handle_SYS_GPU_COMMIT(s64 ctx) {
    gpu_ctx_commit(ctx);
    return 0;
}

s64 handle_SYS_GPU_CLEAR(s64 ctx, u32 rgba_color) {
    gpu_ctx_clear(ctx, rgba_color);
    return 0;
}

s64 handle_SYS_FILE_SIZE(const char *upath) {
    u64            sscratch;
    Process_Frame *frame;
    char           path[256];
    File          *f;

    CSR_READ(sscratch, "sscratch");
    frame = (void*)sscratch;

    user_to_kernel(path, upath, sizeof(path));

    f = get_file(path);

    if (f == NULL) {
        frame->gpregs[XREG_A0] = -1;
    } else {
        frame->gpregs[XREG_A0] = file_size(f);
    }

    return 0;
}

s64 handle_SYS_MAP_MEM(u64 size) {
    void          *pages;
    Process       *current;
    u64            sscratch;
    Process_Frame *frame;

    size    = ALIGN(size, PAGE_SIZE);
    pages   = alloc_pages(size / PAGE_SIZE);
    current = sched_current(sbicall(SBI_HART_ID));

    mmu_map(current->page_table, (u64)pages, current->virt_avail, size, PAGE_READ | PAGE_WRITE | PAGE_USER);

    CSR_READ(sscratch, "sscratch");
    frame = (void*)sscratch;

    frame->gpregs[XREG_A0] = current->virt_avail;

    current->virt_avail += size;

    return 0;
}

s64 handle_SYS_FILE_READ(const char *upath, u8 *udst, u64 offset, u64 n_bytes) {
    u64            sscratch;
    Process_Frame *frame;
    char           path[256];
    File          *f;
    u8            *buff;

    CSR_READ(sscratch, "sscratch");
    frame = (void*)sscratch;

    user_to_kernel(path, upath, sizeof(path));

    f = get_file(path);

    if (f == NULL) {
        frame->gpregs[XREG_A0] = -1;
    } else {
        buff = kmalloc(n_bytes);
        file_read(f, buff, offset, n_bytes);
        kernel_to_user(udst, buff, n_bytes);
        frame->gpregs[XREG_A0] = 0;
    }

    return 0;
}

typedef s64 (*syscall_handler_t)();

#define SYSCALL_FN(s, d) (syscall_handler_t)handle_##s,
syscall_handler_t syscall_handlers[NUM_SYSCALL] = {
    LIST_SYSCALL(SYSCALL_FN)
};
#undef SYSCALL_FN

s64 do_syscall(u32 call) {
    u64            sscratch;
    Process_Frame *frame;
    s64            ret;
    u64            sepc;

    if (call >= NUM_SYSCALL) {
        kprint("%c[syscall]%_ %rbad syscall number (%u)%_\n", call);
        return -1;
    }

    CSR_READ(sscratch, "sscratch");
    frame = (void*)sscratch;

    frame->gpregs[XREG_A0] = 0;

    CSR_READ(sepc, "sepc");
    CSR_WRITE("sepc", sepc + 4);
    frame->sepc = sepc + 4;

    ret = syscall_handlers[call](
            frame->gpregs[XREG_A1],
            frame->gpregs[XREG_A2],
            frame->gpregs[XREG_A3],
            frame->gpregs[XREG_A4],
            frame->gpregs[XREG_A5],
            frame->gpregs[XREG_A6]);

    return ret;
}

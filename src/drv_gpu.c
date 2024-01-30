#include "drv_gpu.h"
#include "kmalloc.h"
#include "virtio.h"
#include "mmu.h"
#include "kprint.h"
#include "machine.h"
#include "utils.h"
#include "input.h"


static DRV_INIT_FN(init, drv_state);
static DRV_IRQ_FN(irq, drv_state);
static DRV_GPU_RESET_DISPLAY_FN(reset_display, drv_state);
static DRV_GPU_CLEAR_FN(clear, drv_state, rgba_color);
static DRV_GPU_GET_RECT_FN(get_rect, drv_state, x, y, w, h);
static DRV_GPU_PIXELS_FN(pixels, drv_state, x, y, w, h, pixels);
static DRV_GPU_RECT_FN(rect, drv_state, x, y, w, h, rgba_color);
static DRV_GPU_COMMIT_FN(commit, drv_state);

Driver DRIVER_GPU = {
    .name              = "virtio-gpu",
    .device_id         = PCI_TO_DEVICE_ID(0x1AF4, 0x1050),
    .type              = DRV_GPU,
    .init              = init,
    .irq               = irq,
    .gpu.reset_display = reset_display,
    .gpu.clear         = clear,
    .gpu.get_rect      = get_rect,
    .gpu.pixels        = pixels,
    .gpu.rect          = rect,
    .gpu.commit        = commit,
};


#define VIRTIO_GPU_EVENT_DISPLAY (1)

typedef struct {
    u32 events_read;
    u32 events_clear;
    u32 num_scanouts;
    u32 reserved;
} VirtIO_GPU_Config;


#define VIRTIO_GPU_FLAG_FENCE (1)

enum {
    /* 2D Commands */
    VIRTIO_GPU_CMD_GET_DISPLAY_INFO = 0x0100,
    VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
    VIRTIO_GPU_CMD_RESOURCE_UNREF,
    VIRTIO_GPU_CMD_SET_SCANOUT,
    VIRTIO_GPU_CMD_RESOURCE_FLUSH,
    VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
    VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
    VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING,
    VIRTIO_GPU_CMD_GET_CAPSET_INFO,
    VIRTIO_GPU_CMD_GET_CAPSET,
    VIRTIO_GPU_CMD_GET_EDID,

    /* cursor commands */
    VIRTIO_GPU_CMD_UPDATE_CURSOR = 0x0300,
    VIRTIO_GPU_CMD_MOVE_CURSOR,

    /* success responses */
    VIRTIO_GPU_RESP_OK_NODATA = 0x1100,
    VIRTIO_GPU_RESP_OK_DISPLAY_INFO,
    VIRTIO_GPU_RESP_OK_CAPSET_INFO,
    VIRTIO_GPU_RESP_OK_CAPSET,
    VIRTIO_GPU_RESP_OK_EDID,

    /* error responses */
    VIRTIO_GPU_RESP_ERR_UNSPEC = 0x1200,
    VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY,
    VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER,
};

typedef struct {
    u32 control_type;
    u32 flags;
    u64 fence_id;
    u32 context_id;
    u32 padding;
} Control_Header;


#define VIRTIO_GPU_MAX_SCANOUTS 16

typedef struct {
    u32 x;
    u32 y;
    u32 w;
    u32 h;
} Rect;

typedef struct {
    Rect rect;
    u32  enabled;
    u32  flags;
} Display;


typedef struct {
    Control_Header header;
    Display        displays[VIRTIO_GPU_MAX_SCANOUTS];
} Display_Info_Reponse;


enum {
   B8G8R8A8_UNORM = 1,
   B8G8R8X8_UNORM = 2,
   A8R8G8B8_UNORM = 3,
   X8R8G8B8_UNORM = 4,
   R8G8B8A8_UNORM = 67,
   X8B8G8R8_UNORM = 68,
   A8B8G8R8_UNORM = 121,
   R8G8B8X8_UNORM = 134,
};

typedef struct {
    Control_Header header;      /* VIRTIO_GPU_CMD_RESOURCE_CREATE_2D           */
    u32            resource_id; /* We get to give a unique ID to each resource */
    u32            format;      /* From GpuFormats above                       */
    u32            width;
    u32            height;
} Resource_Create2D_Request;

typedef struct {
    Control_Header header;
    u32            resource_id;
    u32            n_entries;
} Resource_Attach_Backing_Request;

typedef struct {
    Control_Header header;
    u32            resource_id;
    u32            __padding;
} Resource_Detach_Backing_Request;

typedef struct {
    Control_Header header;
    u32            resource_id;
    u32            __padding;
} Resource_Unref_Request;

typedef struct {
    u64 addr;
    u32 length;
    u32 __padding;
} GPU_Mem_Entry;

typedef struct {
    Control_Header header;
    Rect           rect;
    u32            scanout_id;
    u32            resource_id;
} Set_Scanout_Request;

typedef struct {
    Control_Header header;
    Rect           rect;
    u64            offset;
    u32            resource_id;
    u32            __padding;
} Transfer_To_Host_2D_Request;

typedef struct {
    Control_Header header;
    Rect           rect;
    u32            resource_id;
    u32            __padding;
} Resource_Flush_Request;

typedef struct {
    VirtIO_Device_Info          vio_info;
    volatile VirtIO_GPU_Config *vio_gpu_config;
    u32                         cmd_pending;
    Display                     display;
    u32                        *fb;
    u64                         fb_size; /* in pixels */
    u32                         display_updated;
} GPU_State;



#define RECT_AS_ARGS(_rect) (_rect.x), (_rect.y), (_rect.w), (_rect.h)


void * gpu_cmd(GPU_State *state, u32 cmd, ...) {
    va_list                          args;
    Control_Header                  *rq;
    u64                              rq_size;
    Control_Header                  *rs;
    u64                              rs_size;
    Resource_Create2D_Request       *rq_create_2D;
    Resource_Unref_Request          *rq_unref;
    Resource_Attach_Backing_Request *rq_attach;
    Resource_Detach_Backing_Request *rq_detach;
    GPU_Mem_Entry                   *mem_entry;
    Set_Scanout_Request             *rq_set_scanout;
    Transfer_To_Host_2D_Request     *rq_transfer;
    Resource_Flush_Request          *rq_flush;
    u32                              idx;
    u32                              mod;
    u64                              rs_idx;
    u64                              mem_entry_idx;
    u64                              base;
    u64                              offset;
    u64                              mult;
    volatile u32                    *notify_addr;


    va_start(args, cmd);

    rq = NULL;

    switch (cmd) {
        case VIRTIO_GPU_CMD_GET_DISPLAY_INFO:
            rq_size = sizeof(Control_Header);
            rq      = kmalloc(rq_size);
            memset(rq, 0, rq_size);
            rq->control_type = cmd;

            rs_size = sizeof(Display_Info_Reponse);
            rs      = kmalloc(rs_size);
            memset(rs, 0, rs_size);

            break;

        case VIRTIO_GPU_CMD_RESOURCE_CREATE_2D:
            rq_size = sizeof(Resource_Create2D_Request);
            rq      = kmalloc(rq_size);
            memset(rq, 0, rq_size);
            rq->control_type = cmd;

            rq_create_2D              = (void*)rq;
            rq_create_2D->resource_id = va_arg(args, u32);
            rq_create_2D->format      = va_arg(args, u32);
            rq_create_2D->width       = va_arg(args, u32);
            rq_create_2D->height      = va_arg(args, u32);

            rs_size = sizeof(Control_Header);
            rs      = kmalloc(rs_size);
            memset(rs, 0, rs_size);

            break;

        case VIRTIO_GPU_CMD_RESOURCE_UNREF:
            rq_size = sizeof(Resource_Unref_Request);
            rq      = kmalloc(rq_size);
            memset(rq, 0, rq_size);
            rq->control_type = cmd;

            rq_unref              = (void*)rq;
            rq_unref->resource_id = va_arg(args, u32);

            rs_size = sizeof(Control_Header);
            rs      = kmalloc(rs_size);
            memset(rs, 0, rs_size);

            break;

        case VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING:
            rq_size = sizeof(Resource_Attach_Backing_Request);
            rq      = kmalloc(rq_size);
            memset(rq, 0, rq_size);
            rq->control_type = cmd;

            rq_attach              = (void*)rq;
            rq_attach->resource_id = va_arg(args, u32);
            rq_attach->n_entries   = va_arg(args, u32);

            mem_entry = kmalloc(sizeof(GPU_Mem_Entry));
            memset(mem_entry, 0, sizeof(GPU_Mem_Entry));
            mem_entry->addr   = va_arg(args, u64);
            mem_entry->length = va_arg(args, u32);

            rs_size = sizeof(Control_Header);
            rs      = kmalloc(rs_size);
            memset(rs, 0, rs_size);

            break;

        case VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING:
            rq_size = sizeof(Resource_Detach_Backing_Request);
            rq      = kmalloc(rq_size);
            memset(rq, 0, rq_size);
            rq->control_type = cmd;

            rq_detach              = (void*)rq;
            rq_detach->resource_id = va_arg(args, u32);

            rs_size = sizeof(Control_Header);
            rs      = kmalloc(rs_size);
            memset(rs, 0, rs_size);

            break;

        case VIRTIO_GPU_CMD_SET_SCANOUT:
            rq_size = sizeof(Set_Scanout_Request);
            rq      = kmalloc(rq_size);
            memset(rq, 0, rq_size);
            rq->control_type = cmd;

            rq_set_scanout              = (void*)rq;
            rq_set_scanout->rect.x      = va_arg(args, u32);
            rq_set_scanout->rect.y      = va_arg(args, u32);
            rq_set_scanout->rect.w      = va_arg(args, u32);
            rq_set_scanout->rect.h      = va_arg(args, u32);
            rq_set_scanout->scanout_id  = va_arg(args, u32);
            rq_set_scanout->resource_id = va_arg(args, u32);

            rs_size = sizeof(Control_Header);
            rs      = kmalloc(rs_size);
            memset(rs, 0, rs_size);

            break;

        case VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D:
            rq_size = sizeof(Transfer_To_Host_2D_Request);
            rq      = kmalloc(rq_size);
            memset(rq, 0, rq_size);
            rq->control_type = cmd;

            rq_transfer              = (void*)rq;
            rq_transfer->rect.x      = va_arg(args, u32);
            rq_transfer->rect.y      = va_arg(args, u32);
            rq_transfer->rect.w      = va_arg(args, u32);
            rq_transfer->rect.h      = va_arg(args, u32);
            rq_transfer->offset      = va_arg(args, u64);
            rq_transfer->resource_id = va_arg(args, u32);

            rs_size = sizeof(Control_Header);
            rs      = kmalloc(rs_size);
            memset(rs, 0, rs_size);

            break;

        case VIRTIO_GPU_CMD_RESOURCE_FLUSH:
            rq_size = sizeof(Resource_Flush_Request);
            rq      = kmalloc(rq_size);
            memset(rq, 0, rq_size);
            rq->control_type = cmd;

            rq_flush              = (void*)rq;
            rq_flush->rect.x      = va_arg(args, u32);
            rq_flush->rect.y      = va_arg(args, u32);
            rq_flush->rect.w      = va_arg(args, u32);
            rq_flush->rect.h      = va_arg(args, u32);
            rq_flush->resource_id = va_arg(args, u32);

            rs_size = sizeof(Control_Header);
            rs      = kmalloc(rs_size);
            memset(rs, 0, rs_size);

            break;
    }

    va_end(args);

    if (rq == NULL) {
        kprint("%runhandled GPU command %u!%_\n", cmd);
        return NULL;
    }

    idx = state->vio_info.desc_idx;
    mod = state->vio_info.pci_common->queue_size;

    /* Response */
    rs_idx                                      = idx;
    state->vio_info.descriptor_table[idx].addr  = virt_to_phys(kernel_pt, (u64)rs);
    state->vio_info.descriptor_table[idx].len   = rs_size;
    state->vio_info.descriptor_table[idx].flags = VIRTQ_DESC_F_WRITE;
    state->vio_info.descriptor_table[idx].next  = 0;

    idx = state->vio_info.desc_idx = (state->vio_info.desc_idx + 1) % mod;


    /* Mem entry */
    if (cmd == VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING) {
        mem_entry_idx                               = idx;
        state->vio_info.descriptor_table[idx].addr  = virt_to_phys(kernel_pt, (u64)mem_entry);
        state->vio_info.descriptor_table[idx].len   = sizeof(GPU_Mem_Entry);
        state->vio_info.descriptor_table[idx].flags = VIRTQ_DESC_F_NEXT;
        state->vio_info.descriptor_table[idx].next  = rs_idx;

        idx = state->vio_info.desc_idx = (state->vio_info.desc_idx + 1) % mod;
    }


    /* Request */
    state->vio_info.descriptor_table[idx].addr  = virt_to_phys(kernel_pt, (u64)rq);
    state->vio_info.descriptor_table[idx].len   = rq_size;
    state->vio_info.descriptor_table[idx].flags = VIRTQ_DESC_F_NEXT;
    state->vio_info.descriptor_table[idx].next  = (cmd == VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING)
                                                    ? mem_entry_idx
                                                    : rs_idx;

    state->vio_info.driver_ring->ring[state->vio_info.driver_ring->idx % mod]  = idx;
    state->vio_info.driver_ring->idx                                          += 1;

    state->vio_info.desc_idx = (state->vio_info.desc_idx + 1) % mod;


    base   = (u64)state->vio_info.pci_notify_bar;
    offset = state->vio_info.pci_notify->cap.offset;
    mult   = state->vio_info.pci_notify->notify_off_multiplier;

    notify_addr = (void*)(base + offset + state->vio_info.pci_common->queue_notify_off * mult);

    state->cmd_pending = 1;

    *notify_addr = 0;

    while (state->cmd_pending) { WAIT_FOR_INTERRUPT(); }

    kfree(rq);
    if (cmd == VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING) {
        kfree(mem_entry);
    }

    if (rs->control_type >= VIRTIO_GPU_RESP_ERR_UNSPEC) {
        kprint("%rrequest failed for GPU cmd %u!%_\n", cmd);
    }

    return (void*)rs;
}



static void do_flush(GPU_State *state) {
    Control_Header *response;

    response = gpu_cmd(state, VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
                       RECT_AS_ARGS(state->display.rect),
                       0, /* offset */
                       1 /* resource_id */);
    kfree(response);

    response = gpu_cmd(state, VIRTIO_GPU_CMD_RESOURCE_FLUSH,
                       RECT_AS_ARGS(state->display.rect),
                       1 /* resource_id */);
    kfree(response);
}

static void do_reset_display(GPU_State *state) {
    Display_Info_Reponse *display_info;
    Control_Header       *response;
    Input_Event           event;


    display_info = gpu_cmd(state, VIRTIO_GPU_CMD_GET_DISPLAY_INFO);
    memcpy(&state->display, &display_info->displays[0], sizeof(state->display));

    kfree(display_info);

    if (state->fb != NULL) {
        response = gpu_cmd(state, VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING,
                           1 /* resource_id */);
        kfree(response);

        response = gpu_cmd(state, VIRTIO_GPU_CMD_RESOURCE_UNREF,
                           1 /* resource_id */);
        kfree(response);

        kfree(state->fb);
    }


    response = gpu_cmd(state, VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
                       1, /* resource_id */
                       R8G8B8A8_UNORM,
                       state->display.rect.w,
                       state->display.rect.h);
    kfree(response);

    state->fb_size = state->display.rect.w * state->display.rect.h;
    state->fb = kmalloc(4 * state->fb_size);

    for (u64 i = 0; i < state->fb_size; i += 1) {
/*         state->fb[i] = (i * i) | 0xFF000000; */
        state->fb[i] = 0xFF000000;
    }


    response = gpu_cmd(state, VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
                       1, /* resource_id */
                       1, /* n_entries   */
                       virt_to_phys(kernel_pt, (u64)state->fb),
                       4 * state->fb_size);
    kfree(response);

    response = gpu_cmd(state, VIRTIO_GPU_CMD_SET_SCANOUT,
                       RECT_AS_ARGS(state->display.rect),
                       0, /* scanout_id */
                       1 /* resource_id */);
    kfree(response);

    do_flush(state);

    event.type = EV_DISP;
    input_push(&event);

    state->display_updated = 0;
}


static DRV_INIT_FN(init, drv_state) {
    GPU_State            *state;
    u32                   queue_size;

    state = kmalloc(sizeof(GPU_State));
    memset(state, 0, sizeof(*state));

    drv_state->data = state;
    virtio_get_device_info(&state->vio_info, drv_state->platform_info);

    state->vio_gpu_config = state->vio_info.pci_device_specific;

    state->vio_info.pci_common->device_status  = VIRTIO_DEV_STATUS_RESET;
    state->vio_info.pci_common->device_status |= VIRTIO_DEV_STATUS_ACKNOWLEDGE;
    state->vio_info.pci_common->device_status |= VIRTIO_DEV_STATUS_DRIVER;
    state->vio_info.pci_common->device_status |= VIRTIO_DEV_STATUS_FEATURES_OK;

    state->vio_info.pci_common->queue_select = 0;

    queue_size = state->vio_info.pci_common->queue_size;

    state->vio_info.descriptor_table         = kmalloc(16 * queue_size);
    memset(state->vio_info.descriptor_table, 0, 16 * queue_size);
    state->vio_info.pci_common->queue_desc   = virt_to_phys(kernel_pt, (u64)state->vio_info.descriptor_table);
    state->vio_info.desc_idx                 = 0;

    state->vio_info.driver_ring              = kmalloc(6 + 2 * queue_size);
    memset(state->vio_info.driver_ring, 0, 6 + 2 * queue_size);
    state->vio_info.pci_common->queue_driver = virt_to_phys(kernel_pt, (u64)state->vio_info.driver_ring);

    state->vio_info.device_ring              = kmalloc(6 + 8 * queue_size);
    memset(state->vio_info.device_ring, 0, 6 + 8 * queue_size);
    state->vio_info.pci_common->queue_device = virt_to_phys(kernel_pt, (u64)state->vio_info.device_ring);
    state->vio_info.used_idx                 = 0;


    state->vio_info.pci_common->queue_enable = 1;

    state->vio_info.pci_common->device_status |= VIRTIO_DEV_STATUS_DRIVER_OK;

    do_reset_display(state);

    return 0;
}

static DRV_IRQ_FN(irq, drv_state) {
    GPU_State *state;

    state = drv_state->data;

    if (state->vio_info.pci_isr->queue_interrupt) {
        if (state->vio_gpu_config->events_read) {
            state->vio_gpu_config->events_clear |= VIRTIO_GPU_EVENT_DISPLAY;

            state->display_updated = 1;
        }

        while (state->vio_info.used_idx != state->vio_info.device_ring->idx) {
            state->cmd_pending = 0;

            state->vio_info.used_idx += 1;
        }

        return 0;
    }

    return -1;
}

static DRV_GPU_RESET_DISPLAY_FN(reset_display, drv_state) {
    GPU_State *state;

    state = drv_state->data;

    do_reset_display(state);

    return 0;
}

static DRV_GPU_CLEAR_FN(clear, drv_state, rgba_color) {
    GPU_State *state;
    u64        i;

    state = drv_state->data;

    for (i = 0; i < state->fb_size; i += 1) {
        state->fb[i] = rgba_color;
    }

    return 0;
}

static DRV_GPU_GET_RECT_FN(get_rect, drv_state, x, y, w, h) {
    GPU_State *state;

    state = drv_state->data;

    *x = state->display.rect.x;
    *y = state->display.rect.y;
    *w = state->display.rect.w;
    *h = state->display.rect.h;

    return 0;
}

static DRV_GPU_PIXELS_FN(pixels, drv_state, x, y, w, h, pixels) {
    GPU_State *state;
    u64        X;
    u64        Y;

    state = drv_state->data;

    for (X = 0; X < w && x + X < state->display.rect.w; X += 1) {
        for (Y = 0; Y < h && y + Y < state->display.rect.h; Y += 1) {
            state->fb[(y + Y) * state->display.rect.w + (x + X)] = pixels[X * w + Y];
        }
    }

    return 0;
}

static DRV_GPU_RECT_FN(rect, drv_state, x, y, w, h, rgba_color) {
    GPU_State *state;
    u64        X;
    u64        Y;

    state = drv_state->data;

    for (X = 0; X < w && x + X < state->display.rect.w; X += 1) {
        for (Y = 0; Y < h && y + Y < state->display.rect.h; Y += 1) {
            state->fb[(y + Y) * state->display.rect.w + (x + X)] = rgba_color;
        }
    }

    return 0;
}

static DRV_GPU_COMMIT_FN(commit, drv_state) {
    GPU_State *state;

    state = drv_state->data;

    if (state->display_updated) {
        do_reset_display(state);
    } else {
        do_flush(state);
    }

    return 0;
}

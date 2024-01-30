#include "drv_input.h"
#include "input.h"
#include "kmalloc.h"
#include "virtio.h"
#include "mmu.h"
#include "machine.h"
#include "utils.h"

static DRV_INIT_FN(init, drv_state);
static DRV_IRQ_FN(irq, drv_state);

Driver DRIVER_INPUT = {
    .name        = "virtio-input",
    .device_id   = PCI_TO_DEVICE_ID(0x1AF4, 0x1052),
    .type        = DRV_INPUT,
    .init        = init,
    .irq         = irq,
};

enum {
    VIRTIO_INPUT_CFG_UNSET     = 0x00,
    VIRTIO_INPUT_CFG_ID_NAME   = 0x01,
    VIRTIO_INPUT_CFG_ID_SERIAL = 0x02,
    VIRTIO_INPUT_CFG_ID_DEVIDS = 0x03,
    VIRTIO_INPUT_CFG_PROP_BITS = 0x10,
    VIRTIO_INPUT_CFG_EV_BITS   = 0x11,
    VIRTIO_INPUT_CFG_ABS_INFO  = 0x12,
};

typedef struct {
    u32 min;
    u32 max;
    u32 fuzz;
    u32 flat;
    u32 res;
} Input_Abs_Info;

typedef struct {
    u16 bustype;
    u16 vendor;
    u16 product;
    u16 version;
} Input_Dev_Ids;

typedef struct {
    u8 select;
    u8 subsel;
    u8 size;
    u8 reserved[5];
    union {
        char           string[128];
        u8             bitmap[128];
        Input_Abs_Info abs;
        Input_Dev_Ids  ids;
    };
} Input_Config;


typedef struct {
    VirtIO_Device_Info     vio_info;
    volatile Input_Config *config;
    Input_Event           *events;
} Input_State;

static DRV_INIT_FN(init, drv_state) {
    Input_State *state;
    u32          queue_size;
    u32          i;

    state           = kmalloc(sizeof(Input_State));
    drv_state->data = state;
    virtio_get_device_info(&state->vio_info, drv_state->platform_info);

    state->config = state->vio_info.pci_device_specific;

    state->vio_info.pci_common->device_status  = VIRTIO_DEV_STATUS_RESET;
    state->vio_info.pci_common->device_status |= VIRTIO_DEV_STATUS_ACKNOWLEDGE;
    state->vio_info.pci_common->device_status |= VIRTIO_DEV_STATUS_DRIVER;
    state->vio_info.pci_common->device_status |= VIRTIO_DEV_STATUS_FEATURES_OK;

    state->vio_info.pci_common->queue_select = 0;

    state->vio_info.pci_common->queue_size = 1024;
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

    state->events = kmalloc(queue_size * sizeof(Input_Event));

    for (i = 0; i < queue_size; i += 1) {
        state->vio_info.descriptor_table[i].addr  = virt_to_phys(kernel_pt, (u64)(state->events + i));
        state->vio_info.descriptor_table[i].len   = sizeof(Input_Event);
        state->vio_info.descriptor_table[i].flags = VIRTQ_DESC_F_WRITE;
        state->vio_info.descriptor_table[i].next  = 0;

        state->vio_info.driver_ring->ring[state->vio_info.driver_ring->idx % queue_size] = i;
        state->vio_info.desc_idx = (state->vio_info.desc_idx + 1) % queue_size;

        state->vio_info.driver_ring->idx += 1;
    }

    return 0;
}

static DRV_IRQ_FN(irq, drv_state) {
    Input_State          *state;
    u32                   queue_size;
    u32                   idx;
    VirtQ_Used_Ring_Elem *elem;
    Input_Event          *event;

    state = drv_state->data;

    if (state->vio_info.pci_isr->queue_interrupt) {
        queue_size = state->vio_info.pci_common->queue_size;

        while (state->vio_info.used_idx != state->vio_info.device_ring->idx) {
            idx   = state->vio_info.used_idx % queue_size;
            elem  = state->vio_info.device_ring->ring + idx;
            event = state->events + elem->id;

            if (event->type == EV_KEY) {
                input_push(event);
            } else if (event->type == EV_ABS) {
                input_push(event);
            }

            state->vio_info.driver_ring->idx += 1;
            state->vio_info.used_idx         += 1;
        }

        return 0;
    }

    return -1;
}

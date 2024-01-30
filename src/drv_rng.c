#include "drv_rng.h"
#include "kmalloc.h"
#include "virtio.h"
#include "mmu.h"
#include "kprint.h"
#include "machine.h"
#include "utils.h"

static DRV_INIT_FN(init, drv_state);
static DRV_IRQ_FN(irq, drv_state);
static DRV_RNG_SERVICE_FN(service, drv_state, buffer, len);

Driver DRIVER_RNG = {
    .name        = "virtio-rng",
    .device_id   = PCI_TO_DEVICE_ID(0x1AF4, 0x1044),
    .type        = DRV_RNG,
    .init        = init,
    .irq         = irq,
    .rng.service = service,
};

typedef struct {
    VirtIO_Device_Info vio_info;
    u32                in_flight;
} RNG_State;

static DRV_INIT_FN(init, drv_state) {
    RNG_State *state;
    u32        queue_size;

    state           = kmalloc(sizeof(RNG_State));
    drv_state->data = state;
    virtio_get_device_info(&state->vio_info, drv_state->platform_info);

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

    return 0;
}

static DRV_IRQ_FN(irq, drv_state) {
    RNG_State *state;

    state = drv_state->data;

    if (state->vio_info.pci_isr->queue_interrupt) {
        while (state->vio_info.used_idx != state->vio_info.device_ring->idx) {
            state->vio_info.used_idx += 1;
        }

        state->in_flight = 0;

        return 0;
    }

    return -1;
}

static DRV_RNG_SERVICE_FN(service, drv_state, buffer, len) {
    RNG_State    *state;
    u32           idx;
    u32           mod;
    u64           buff_phys;
    u64           base;
    u64           offset;
    u64           mult;
    volatile u32 *notify_addr;

    state = drv_state->data;


    state->in_flight = 1;

    idx       = state->vio_info.desc_idx;
    mod       = state->vio_info.pci_common->queue_size;
    buff_phys = virt_to_phys(kernel_pt, (u64)buffer);

    state->vio_info.descriptor_table[idx].addr  = buff_phys;
    state->vio_info.descriptor_table[idx].len   = len;
    state->vio_info.descriptor_table[idx].flags = VIRTQ_DESC_F_WRITE;
    state->vio_info.descriptor_table[idx].next  = 0;

    state->vio_info.driver_ring->ring[state->vio_info.driver_ring->idx % mod]  = idx;
    state->vio_info.driver_ring->idx                                          += 1;

    state->vio_info.desc_idx = (state->vio_info.desc_idx + 1) % mod;

    base   = (u64)state->vio_info.pci_notify_bar;
    offset = state->vio_info.pci_notify->cap.offset;
    mult   = state->vio_info.pci_notify->notify_off_multiplier;

    notify_addr  = (void*)(base + offset + state->vio_info.pci_common->queue_notify_off * mult);
    *notify_addr = 0;

    while (state->in_flight) { WAIT_FOR_INTERRUPT(); }

    return 0;
}

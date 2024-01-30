#include "drv_rng.h"
#include "kmalloc.h"
#include "virtio.h"
#include "mmu.h"
#include "kprint.h"
#include "machine.h"
#include "utils.h"

static DRV_INIT_FN(init, drv_state);
static DRV_IRQ_FN(irq, drv_state);
static DRV_BLK_READ_FN(read, drv_state, offset, buffer, len);
static DRV_BLK_WRITE_FN(write, drv_state, offset, buffer, len);

Driver DRIVER_BLK = {
    .name        = "virtio-block",
    .device_id   = PCI_TO_DEVICE_ID(0x1AF4, 0x1042),
    .type        = DRV_BLOCK,
    .init        = init,
    .irq         = irq,
    .block.read  = read,
    .block.write = write,
};

typedef struct {
   u64 capacity;
   u32 size_max;
   u32 seg_max;

   struct {
      u16 cylinders;
      u8  heads;
      u8  sectors;
   } geometry;

   u32 blk_size; /* the size of a sector, usually 512 */

   struct {
      u8  physical_block_exp; /* # of logical blocks per physical block (log2)  */
      u8  alignment_offset;   /* offset of first aligned logical block          */
      u16 min_io_size;        /* suggested minimum I/O size in blocks           */
      u32 opt_io_size;        /* optimal (suggested maximum) I/O size in blocks */
   } topology;

   u8  writeback;
   u8  unused0[3];
   u32 max_discard_sectors;
   u32 max_discard_seg;
   u32 discard_sector_alignment;
   u32 max_write_zeroes_sectors;
   u32 max_write_zeroes_seg;
   u8  write_zeroes_may_unmap;
   u8  unused1[3];
} VirtIO_Block_Config;

#define VIRTIO_BLK_T_IN           (0)
#define VIRTIO_BLK_T_OUT          (1)
#define VIRTIO_BLK_T_FLUSH        (4)
#define VIRTIO_BLK_T_DISCARD      (11)
#define VIRTIO_BLK_T_WRITE_ZEROES (13)

#define VIRTIO_BLK_S_OK           (0)
#define VIRTIO_BLK_S_IOERR        (1)
#define VIRTIO_BLK_S_UNSUPP       (2)

enum {
    BLK_NONE_IN_FLIGHT,
    BLK_READ,
    BLK_WRITE,
};

typedef struct {
    VirtIO_Device_Info            vio_info;
    volatile VirtIO_Block_Config *vio_blk_config;
    u32                           in_flight;
    u8                           *buff_virt;
    u64                           len;
    u8                           *dst_buff_virt;
    u64                           blk_offset;
} Block_State;

typedef struct {
    u32 type;      /* IN/OUT                             */
    u32 reserved;
    u64 sector;    /* start sector (LBA / cfg->blk_size) */
} Request_Header;

typedef struct {
    /* u8 bytes[]; Multiple of cfg->blk_size */
} Request_Data;

typedef struct {
    u8 status;
} Request_Status;

static DRV_INIT_FN(init, drv_state) {
    Block_State *state;
    u32        queue_size;

    state = kmalloc(sizeof(Block_State));
    memset(state, 0, sizeof(*state));

    drv_state->data = state;
    virtio_get_device_info(&state->vio_info, drv_state->platform_info);

    state->vio_blk_config = state->vio_info.pci_device_specific;

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
    Block_State          *state;
    VirtQ_Used_Ring_Elem  used;
    VirtIO_Descriptor    *desc;
    Request_Status       *status;

    state = drv_state->data;

    if (state->vio_info.pci_isr->queue_interrupt) {
        while (state->vio_info.used_idx != state->vio_info.device_ring->idx) {
            used = state->vio_info.device_ring->ring[state->vio_info.used_idx];

            /* Header */
            desc = state->vio_info.descriptor_table + used.id;
            /* Data */
            desc = state->vio_info.descriptor_table + desc->next;
            /* Status */
            desc = state->vio_info.descriptor_table + desc->next;

            status = (void*)desc->addr;

            switch (status->status) {
                case VIRTIO_BLK_S_OK:
                    if (state->in_flight == BLK_READ) {
                        memcpy(state->dst_buff_virt, state->buff_virt + state->blk_offset, state->len);
                    } else if (state->in_flight == BLK_WRITE) {
                    }
                    break;
                case VIRTIO_BLK_S_IOERR:
                    kprint("virtio-blk: IOERR\n");
                    break;
                case VIRTIO_BLK_S_UNSUPP:
                    kprint("virtio-blk: UNSUPP\n");
                    break;
                case 123:
                    kprint("virtio-blk: status unchanged\n");
                    break;
                default:
                    kprint("virtio-blk: unknown status\n");
                    break;
            }

            state->vio_info.used_idx += 1;
        }

        state->in_flight = 0;

        return 0;
    }

    return -1;
}

static DRV_BLK_READ_FN(read, drv_state, offset, buffer, len) {
    Block_State    *state;
    u64             blk_size;
    u64             bytes_needed;
    u32             idx;
    u32             mod;
    Request_Status *rq_status;
    u32             idx_status;
    Request_Data   *rq_data;
    u32             idx_data;
    Request_Header *rq_header;
    u64             notif_base;
    u64             notif_offset;
    u64             notif_mult;
    volatile u32   *notify_addr;

    state = drv_state->data;


    blk_size     = state->vio_blk_config->blk_size;
    bytes_needed = blk_size * (1 + (len / blk_size));


    state->buff_virt     = kmalloc(bytes_needed);
    state->len           = len;
    state->dst_buff_virt = buffer;
    state->blk_offset    = offset % blk_size;


    idx = state->vio_info.desc_idx;
    mod = state->vio_info.pci_common->queue_size;


    /* Status */
    rq_status = kmalloc(sizeof(*rq_status));
    rq_status->status = 123;

    idx_status                                  = idx;
    state->vio_info.descriptor_table[idx].addr  = virt_to_phys(kernel_pt, (u64)rq_status);
    state->vio_info.descriptor_table[idx].len   = sizeof(*rq_status);
    state->vio_info.descriptor_table[idx].flags = VIRTQ_DESC_F_WRITE;
    state->vio_info.descriptor_table[idx].next  = 0;

    idx = state->vio_info.desc_idx = (state->vio_info.desc_idx + 1) % mod;

    /* Data */
    rq_data = (void*)state->buff_virt;

    idx_data                                    = idx;
    state->vio_info.descriptor_table[idx].addr  = virt_to_phys(kernel_pt, (u64)rq_data);
    state->vio_info.descriptor_table[idx].len   = bytes_needed;
    state->vio_info.descriptor_table[idx].flags = VIRTQ_DESC_F_WRITE | VIRTQ_DESC_F_NEXT;
    state->vio_info.descriptor_table[idx].next  = idx_status;

    idx = state->vio_info.desc_idx = (state->vio_info.desc_idx + 1) % mod;

    /* Header */
    rq_header = kmalloc(sizeof(*rq_header));
    memset(rq_status, 0, sizeof(*rq_header));

    rq_header->type   = VIRTIO_BLK_T_IN;
    rq_header->sector = offset / blk_size;

    state->vio_info.descriptor_table[idx].addr  = virt_to_phys(kernel_pt, (u64)rq_header);
    state->vio_info.descriptor_table[idx].len   = sizeof(*rq_header);
    state->vio_info.descriptor_table[idx].flags = VIRTQ_DESC_F_NEXT;
    state->vio_info.descriptor_table[idx].next  = idx_data;


    state->vio_info.driver_ring->ring[state->vio_info.driver_ring->idx % mod]  = idx;
    state->vio_info.driver_ring->idx                                          += 1;

    state->vio_info.desc_idx = (state->vio_info.desc_idx + 1) % mod;


    notif_base   = (u64)state->vio_info.pci_notify_bar;
    notif_offset = state->vio_info.pci_notify->cap.offset;
    notif_mult   = state->vio_info.pci_notify->notify_off_multiplier;
    notify_addr  = (void*)(notif_base + notif_offset + state->vio_info.pci_common->queue_notify_off * notif_mult);

    state->in_flight = BLK_READ;

    *notify_addr = 0;

    while (state->in_flight) { WAIT_FOR_INTERRUPT(); }

    kfree(rq_header);
    kfree(rq_data);
    kfree(rq_status);

    return 0;
}

static DRV_BLK_WRITE_FN(write, drv_state, offset, buffer, len) {
    Block_State    *state;
    u64             blk_size;
    u64             bytes_needed;
    u8             *first_block;
    u8             *last_block;
    u32             idx;
    u32             mod;
    Request_Status *rq_status;
    u32             idx_status;
    Request_Data   *rq_data;
    u32             idx_data;
    Request_Header *rq_header;
    u64             notif_base;
    u64             notif_offset;
    u64             notif_mult;
    volatile u32   *notify_addr;

    state = drv_state->data;


    blk_size     = state->vio_blk_config->blk_size;
    bytes_needed = blk_size * (1 + (len / blk_size));

    first_block = kmalloc(blk_size);
    read(drv_state, blk_size * (offset / blk_size), first_block, blk_size);

    last_block  = kmalloc(blk_size);
    read(drv_state, blk_size * ((offset + len) / blk_size), last_block, blk_size);

    state->buff_virt     = kmalloc(bytes_needed);
    state->len           = len;
    state->dst_buff_virt = NULL;
    state->blk_offset    = offset % blk_size;

    memcpy(state->buff_virt,                           first_block, blk_size);
    memcpy(state->buff_virt + bytes_needed - blk_size, last_block,  blk_size);
    memcpy(state->buff_virt + state->blk_offset,       buffer,      len);

    kfree(last_block);
    kfree(first_block);


    idx = state->vio_info.desc_idx;
    mod = state->vio_info.pci_common->queue_size;


    /* Status */
    rq_status         = kmalloc(sizeof(*rq_status));
    rq_status->status = 123;

    idx_status                                  = idx;
    state->vio_info.descriptor_table[idx].addr  = virt_to_phys(kernel_pt, (u64)rq_status);
    state->vio_info.descriptor_table[idx].len   = sizeof(*rq_status);
    state->vio_info.descriptor_table[idx].flags = VIRTQ_DESC_F_WRITE;
    state->vio_info.descriptor_table[idx].next  = 0;

    idx = state->vio_info.desc_idx = (state->vio_info.desc_idx + 1) % mod;

    /* Data */
    rq_data = (void*)state->buff_virt;

    idx_data                                    = idx;
    state->vio_info.descriptor_table[idx].addr  = virt_to_phys(kernel_pt, (u64)rq_data);
    state->vio_info.descriptor_table[idx].len   = bytes_needed;
    state->vio_info.descriptor_table[idx].flags = VIRTQ_DESC_F_NEXT;
    state->vio_info.descriptor_table[idx].next  = idx_status;

    idx = state->vio_info.desc_idx = (state->vio_info.desc_idx + 1) % mod;

    /* Header */
    rq_header = kmalloc(sizeof(*rq_header));
    memset(rq_status, 0, sizeof(*rq_header));

    rq_header->type   = VIRTIO_BLK_T_OUT;
    rq_header->sector = offset / blk_size;

    state->vio_info.descriptor_table[idx].addr  = virt_to_phys(kernel_pt, (u64)rq_header);
    state->vio_info.descriptor_table[idx].len   = sizeof(*rq_header);
    state->vio_info.descriptor_table[idx].flags = VIRTQ_DESC_F_NEXT;
    state->vio_info.descriptor_table[idx].next  = idx_data;


    state->vio_info.driver_ring->ring[state->vio_info.driver_ring->idx % mod]  = idx;
    state->vio_info.driver_ring->idx                                          += 1;

    state->vio_info.desc_idx = (state->vio_info.desc_idx + 1) % mod;


    notif_base   = (u64)state->vio_info.pci_notify_bar;
    notif_offset = state->vio_info.pci_notify->cap.offset;
    notif_mult   = state->vio_info.pci_notify->notify_off_multiplier;
    notify_addr  = (void*)(notif_base + notif_offset + state->vio_info.pci_common->queue_notify_off * notif_mult);

    state->in_flight = BLK_WRITE;

    *notify_addr = 0;

    while (state->in_flight) { WAIT_FOR_INTERRUPT(); }

    kfree(rq_header);
    kfree(rq_data);
    kfree(rq_status);

    return 0;
}

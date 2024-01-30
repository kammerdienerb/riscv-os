#ifndef __VIRTIO_H__
#define __VIRTIO_H__

#include "common.h"


#define VIRTIO_PCI_CAP_COMMON_CFG (1)  /* Common configuration          */
#define VIRTIO_PCI_CAP_NOTIFY_CFG (2)  /* Notifications                 */
#define VIRTIO_PCI_CAP_ISR_CFG    (3)  /* ISR Status                    */
#define VIRTIO_PCI_CAP_DEVICE_CFG (4)  /* Device specific configuration */
#define VIRTIO_PCI_CAP_PCI_CFG    (5)  /* PCI configuration access      */

#define VIRTIO_DEV_STATUS_RESET       (0)
#define VIRTIO_DEV_STATUS_ACKNOWLEDGE (1 << 0)
#define VIRTIO_DEV_STATUS_DRIVER      (1 << 1)
#define VIRTIO_DEV_STATUS_DRIVER_OK   (1 << 2)
#define VIRTIO_DEV_STATUS_FEATURES_OK (1 << 3)

#define BAR_NOTIFY_CAP(offset, queue_notify_off, notify_off_multiplier) \
    ((offset) + (queue_notify_off) * (notify_off_multiplier))

typedef struct {
    u8  cap_vndr;   /* Generic PCI field: PCI_CAP_ID_VNDR   */
    u8  cap_next;   /* Generic PCI field: next ptr.         */
    u8  cap_len;    /* Generic PCI field: capability length */
    u8  cfg_type;   /* Identifies the structure.            */
    u8  bar;        /* Which BAR to find it.                */
    u8  padding[3]; /* Pad to full dword.                   */
    u32 offset;     /* Offset within bar.                   */
    u32 length;     /* Length of the structure, in bytes.   */
} VirtIO_PCI_Capability;

typedef struct {
    u32 device_feature_select; /* read-write           */
    u32 device_feature;        /* read-only for driver */
    u32 driver_feature_select; /* read-write           */
    u32 driver_feature;        /* read-write           */
    u16 msix_config;           /* read-write           */
    u16 num_queues;            /* read-only for driver */
    u8  device_status;         /* read-write           */
    u8  config_generation;     /* read-only for driver */

    /* About a specific virtqueue. */
    u16 queue_select;          /* read-write           */
    u16 queue_size;            /* read-write           */
    u16 queue_msix_vector;     /* read-write           */
    u16 queue_enable;          /* read-write           */
    u16 queue_notify_off;      /* read-only for driver */
    u64 queue_desc;            /* read-write           */
    u64 queue_driver;          /* read-write           */
    u64 queue_device;          /* read-write           */
} VirtIO_PCI_Common_Config;

typedef struct {
    VirtIO_PCI_Capability cap;
    u32                   notify_off_multiplier; /* Multiplier for queue_notify_off. */
} VirtIO_PCI_Notify_Capability;

typedef struct {
    union {
       struct {
          unsigned queue_interrupt:      1;
          unsigned device_cfg_interrupt: 1;
          unsigned reserved:             30;
       };
       unsigned int isr_cap;
    };
} VirtIO_PCI_ISR_Capability;



#define VIRTQ_DESC_F_NEXT     (1)  /* This marks a buffer as continuing via the next field.                  */
#define VIRTQ_DESC_F_WRITE    (2)  /* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTQ_DESC_F_INDIRECT (4)  /* This means the buffer contains a list of buffer descriptors.           */

typedef struct {
   u64 addr;   /* Address (guest-physical).     */
   u32 len;    /* Length.                       */
   u16 flags;  /* The flags as indicated above. */
   u16 next;   /* Next field if flags & NEXT    */
} VirtIO_Descriptor;



#define VIRTQ_AVAIL_F_NO_INTERRUPT (1)

typedef struct {
   u16 flags;
   u16 idx;
   u16 ring[ /* Queue Size */ ];
/*    u16 used_event;  Only if VIRTIO_F_EVENT_IDX */
} VirtQ_Available_Ring;

typedef struct {
   u32 id;  /* Index of start of used descriptor chain.                         */
   u32 len; /* Total length of the descriptor chain which was used (written to) */
} VirtQ_Used_Ring_Elem;

#define VIRTQ_USED_F_NO_NOTIFY (1)

typedef struct {
   u16                  flags;
   u16                  idx;
   VirtQ_Used_Ring_Elem ring[ /* Queue Size */ ];
/*    u16                  avail_event; Only if VIRTIO_F_EVENT_IDX */
} VirtQ_Used_Ring;


typedef struct {
    volatile VirtIO_PCI_Common_Config     *pci_common;
    volatile void                         *pci_common_bar;
    volatile VirtIO_PCI_Notify_Capability *pci_notify;
    volatile void                         *pci_notify_bar;
    volatile VirtIO_PCI_ISR_Capability    *pci_isr;
    volatile void                         *pci_isr_bar;
    volatile void                         *pci_device_specific;
    VirtIO_Descriptor                     *descriptor_table;
    u32                                    desc_idx;
    VirtQ_Available_Ring                  *driver_ring;
    VirtQ_Used_Ring                       *device_ring;
    u16                                    used_idx;
} VirtIO_Device_Info;

void virtio_get_device_info(VirtIO_Device_Info *info, void *pci_ecam);


#endif

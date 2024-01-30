#include "virtio.h"
#include "pci.h"
#include "kmalloc.h"
#include "kprint.h"

void virtio_get_device_info(VirtIO_Device_Info *info, void *pci_ecam) {
    volatile PCI_Ecam              *ecam;
    volatile PCIE_Capability       *cap;
    u8                              cap_next;
    volatile VirtIO_PCI_Capability *vio_cap;
    void                           *addr;
    volatile u32                   *barp;
    volatile u64                   *barp64;

    ecam = pci_ecam;

    if (ecam->status_reg & (1 << 4)) {
        cap_next = ecam->type0.capes_pointer;
        while (cap_next) {
            cap = ((void*)ecam) + cap_next;

            if (cap->id == 0x9) {
                vio_cap = (void*)cap;

                addr = NULL;

                barp = ecam->type0.bar + vio_cap->bar;

                if (((*barp >> 1) & 0x3) == 0x0) { /* 32-bit */
                    addr = (void*)(((u64)*barp) & ~0xF);
                } else if (((*barp >> 1) & 0x3) == 0x2) { /* 64-bit */
                    barp64 = (u64*)(void*)(ecam->type0.bar + vio_cap->bar);
                    addr = (void*)((*barp64) & ~0xFULL);
                }

                if (addr != NULL) {
                    switch (vio_cap->cfg_type) {
                        case VIRTIO_PCI_CAP_COMMON_CFG:
                            info->pci_common     = addr + vio_cap->offset;
                            info->pci_common_bar = addr;
                            break;
                        case VIRTIO_PCI_CAP_NOTIFY_CFG:
                            info->pci_notify     = (void*)cap;
                            info->pci_notify_bar = addr;
                            break;
                        case VIRTIO_PCI_CAP_ISR_CFG:
                            info->pci_isr     = addr + vio_cap->offset;
                            info->pci_isr_bar = addr;
                            break;
                        case VIRTIO_PCI_CAP_DEVICE_CFG:
                            info->pci_device_specific = addr + vio_cap->offset;
                            break;
                    }
                }
            }

            cap_next = cap->next;
        }
    }
}

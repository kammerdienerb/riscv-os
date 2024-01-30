#ifndef __PCI_H__
#define __PCI_H__

#include "common.h"
#include "irq.h"

typedef struct {
    u16 vendor_id;
    u16 device_id;
    u16 command_reg;
    u16 status_reg;
    u8  revision_id;
    u8  prog_if;
    union {
        u16 class_code;
        struct {
            u8 class_subcode;
            u8 class_basecode;
        };
    };
    u8 cacheline_size;
    u8 latencyimer;
    u8 header_type;
    u8 bist;
    union {
        struct {
            u32 bar[6];
            u32 cardbus_cis_pointer;
            u16 sub_vendor_id;
            u16 sub_device_id;
            u32 expansion_rom_addr;
            u8  capes_pointer;
            u8  reserved0[3];
            u32 reserved1;
            u8  interrupt_line;
            u8  interrupt_pin;
            u8  min_gnt;
            u8  max_lat;
        } type0;
        struct {
            u32 bar[2];
            u8  primary_bus_no;
            u8  secondary_bus_no;
            u8  subordinate_bus_no;
            u8  secondary_latency_timer;
            u8  io_base;
            u8  io_limit;
            u16 secondary_status;
            u16 memory_base;
            u16 memory_limit;
            u16 prefetch_memory_base;
            u16 prefetch_memory_limit;
            u32 prefetch_base_upper;
            u32 prefetch_limit_upper;
            u16 io_base_upper;
            u16 io_limit_upper;
            u8  capes_pointer;
            u8  reserved0[3];
            u32 expansion_rom_addr;
            u8  interrupt_line;
            u8  interrupt_pin;
            u16 bridge_control;
        } type1;
        struct {
            u32 reserved0[9];
            u8  capes_pointer;
            u8  reserved1[3];
            u32 reserved2;
            u8  interrupt_line;
            u8  interrupt_pin;
            u8  reserved3[2];
        } common;
    };
} PCI_Ecam;

typedef struct {
    u8 id;
    u8 next;
} PCIE_Capability;


#define PCIE_COMMAND_MEM_SPACE  (1 << 1)
#define PCIE_COMMAND_BUS_MASTER (1 << 2)

#define PCI_DEVICE_IRQ_NUMBER(_bus, _slot) \
    (IRQ_ENC(INT_PCIA + (((_bus) + (_slot)) & 0x3), 1))

void      init_pci(void);
PCI_Ecam *pci_device_ecam_addr(u8 bus, u8 device, u8 function, u16 reg);
s64       pci_irq_dispatch(u32 hartid, u64 cause);

#endif

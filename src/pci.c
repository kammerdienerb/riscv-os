#include "pci.h"
#include "machine.h"
#include "driver.h"
#include "kprint.h"

void init_pci(void) {
    u32                bus;
    u32                device;
    volatile PCI_Ecam *ecam;
    u32                pci_bus;
    u32                n_bars;
    void              *bar_mem;
    u32                b;
    volatile u32      *barp;
    volatile u64      *barp64;
    u32                bar_space;
    Driver            *driver;


    pci_bus = 1; /* 0 is already taken by the root bus. */

    /* Set up bridges. */
    for (bus = 0; bus < 256; bus += 1) {
        for (device = 0; device < 32; device += 1) {
            ecam = pci_device_ecam_addr(bus, device, 0, 0);
            if (ecam->vendor_id == 0xffff) { continue; }

            if (ecam->header_type == 1) {
                ecam->command_reg                 = PCIE_COMMAND_MEM_SPACE | PCIE_COMMAND_BUS_MASTER; // bits 1 and 2
                ecam->type1.memory_base           = 0x4000;
                ecam->type1.memory_limit          = 0x4fff;
                ecam->type1.prefetch_memory_limit = 0x4fff;
                ecam->type1.prefetch_memory_base  = 0x4000;
                ecam->type1.primary_bus_no        = bus;
                ecam->type1.secondary_bus_no      = pci_bus;
                ecam->type1.subordinate_bus_no    = pci_bus;

                pci_bus += 1;
            }
        }
    }

    for (bus = 0; bus < 256; bus += 1) {
        for (device = 0; device < 32; device += 1) {
            ecam = pci_device_ecam_addr(bus, device, 0, 0);
            if (ecam->vendor_id == 0xffff) { continue; }
            if (ecam->header_type != 0)    { continue; }

            /* Set up bars */

            bar_mem = (void*)(BAR_BASE | (bus << 20ULL) | (device << 16ULL));
            n_bars  = (ecam->header_type ? 2 : 6);

            for (b = 0; b < n_bars; b += 1) {
                barp = ecam->type0.bar + b;

                if (((*barp >> 1) & 0x3) == 0x0) { /* 32-bit */
                    ecam->command_reg = 0;

                    *barp = 0xFFFFFFFF;
                    if (*barp == 0) { continue; }

                    bar_space = -(*barp & ~0xF);

                    ecam->command_reg = PCIE_COMMAND_MEM_SPACE;

                    if (bar_space != 0) {
                        if (!IS_ALIGNED(bar_mem, bar_space)) { bar_mem = ALIGN(bar_mem, bar_space); }
                        *barp    = (u32)(u64)bar_mem;
                        bar_mem += bar_space;
                    }
                } else if (((*barp >> 1) & 0x3) == 0x2) { /* 64-bit */
                    barp64 = (volatile u64*)(void*)barp;

                    ecam->command_reg = 0;

                    *barp64 = 0xFFFFFFFFFFFFFFFF;
                    if (barp64 == 0) { continue; }

                    bar_space = -(*barp64 & ~0xF);

                    ecam->command_reg = PCIE_COMMAND_MEM_SPACE;

                    if (bar_space != 0) {
                        if (!IS_ALIGNED(bar_mem, bar_space)) { bar_mem = ALIGN(bar_mem, bar_space); }
                        *barp64  = (u64)bar_mem;
                        bar_mem += bar_space;
                    }

                    b += 1;
                }
            }

            if ((driver = find_driver(PCI_TO_DEVICE_ID(ecam->vendor_id, ecam->device_id)))) {
                if (start_driver(driver, PCI_TO_ACTIVE_DEVICE_ID(bus, device), PCI_DEVICE_IRQ_NUMBER(bus, device), (void*)ecam) != 0) {
                    kprint("PCI: %rFailed to start driver for active_device_id 0x%x%_\n", PCI_TO_ACTIVE_DEVICE_ID(bus, device));
                } else {
                    kprint("PCI: %gStarted driver '%s' for active_device_id 0x%x!%_\n", driver->name, PCI_TO_ACTIVE_DEVICE_ID(bus, device));
                }
            } else {
                kprint("PCI: %yDid not find driver for active_device_id 0x%x%_\n", PCI_TO_ACTIVE_DEVICE_ID(bus, device));
            }
        }
    }
}

PCI_Ecam *pci_device_ecam_addr(u8 bus, u8 device, u8 function, u16 reg) {
    return (PCI_Ecam*)(0ULL
            | (ECAM_BASE)          /* base 0x3000_0000                         */
            | (bus      << 20ULL)  /* bus number A[(20+n-1):20] (up to 8 bits) */
            | (device   << 15ULL)  /* device number A[19:15]                   */
            | (function << 12ULL)  /* function umber A[14:12]                  */
            | (reg      << 2ULL)); /* register number A[11:2]                  */
}

s64 pci_irq_dispatch(u32 hartid, u64 irq_number) {
    Driver_State **statep;
    Driver_State  *state;

    FOR_DRIVER_STATE(statep) {
        state = *statep;
        if (state->irq_number == irq_number) {
            if (state->driver->irq(state) == 0) { return 0; }
        }
    }

    return -1;
}

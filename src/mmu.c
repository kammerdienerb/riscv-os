#include "mmu.h"
#include "page.h"
#include "machine.h"
#include "utils.h"
#include "symbols.h"
#include "lock.h"
#include "sbi.h"
#include "sched.h"

#define PTE_PPN0_BIT (10ULL)
#define PTE_PPN1_BIT (19ULL)
#define PTE_PPN2_BIT (28ULL)

#define ADDR_0_BIT (12ULL)
#define ADDR_1_BIT (21ULL)
#define ADDR_2_BIT (30ULL)


#define PTE_IS_BRANCH(pte)    (((pte) & 0xEULL) == 0)
#define PTE_BRANCH_TO_PT(pte) ((Page_Table*)(((pte) << 2ULL) & ~0xFFFULL))



Page_Table *kernel_pt;
static Spinlock kernel_pt_lock;

void init_mmu(void) {
    kernel_pt = alloc_pages(1);
    memset(kernel_pt, 0, sizeof(*kernel_pt));

    kernel_pt_lock.s = SPIN_UNLOCKED;

    mmu_map(kernel_pt, sym_start(text),   sym_start(text),   ALIGN(sym_size(text), PAGE_SIZE),   PAGE_READ | PAGE_EXECUTE);
    mmu_map(kernel_pt, sym_start(data),   sym_start(data),   ALIGN(sym_size(data), PAGE_SIZE),   PAGE_READ | PAGE_WRITE);

    /* What the hell? Why doesn't _bss_end include all of my global data? */
    mmu_map(kernel_pt, sym_start(bss),    sym_start(bss),    ALIGN(sym_start(rodata) - sym_start(bss), PAGE_SIZE),    PAGE_READ | PAGE_WRITE);

    mmu_map(kernel_pt, sym_start(rodata), sym_start(rodata), ALIGN(sym_size(rodata), PAGE_SIZE), PAGE_READ);
    mmu_map(kernel_pt, sym_start(stack),  sym_start(stack),  ALIGN(sym_size(stack), PAGE_SIZE),  PAGE_READ | PAGE_WRITE);
    mmu_map(kernel_pt, sym_start(heap),   sym_start(heap),   ALIGN(sym_size(heap), PAGE_SIZE),   PAGE_READ | PAGE_WRITE);

    /*** MMIO zones ***/

    /* PLIC */
    mmu_map(kernel_pt, 0x0C000000, 0x0C000000, 0x0C300000 - 0x0C000000, PAGE_READ | PAGE_WRITE);

    /* PCIe MMIO */
    mmu_map(kernel_pt, 0x30000000, 0x30000000, 0x10000000, PAGE_READ | PAGE_WRITE);
    /* PCIe BARs */
    mmu_map(kernel_pt, 0x40000000, 0x40000000, 0x10000000, PAGE_READ | PAGE_WRITE);
}

void activate_mmu(void) {
    u64 new_satp;

    new_satp =  SATP_MODE_SV39
              | SATP_SET_ASID(KERNEL_ASID)
              | SATP_SET_PPN(kernel_pt);

    CSR_WRITE("satp", new_satp);
    SFENCE();
}

u64 mmu_map(Page_Table *pt, u64 paddr, u64 vaddr, u64 size, u64 bits) {
    Page_Table *pt_save;
    u64         mapped;
    u64         vpn[3];
    u64         ppn[3];
    u32         level;
    u64         pte;
    Page_Table *new_pt;

    pt_save = pt;
    mapped  = 0;

    if (!IS_ALIGNED(size,  PAGE_SIZE)
    ||  !IS_ALIGNED(paddr, PAGE_SIZE)
    ||  !IS_ALIGNED(vaddr, PAGE_SIZE)
    ||  size == 0) {

        goto out;
    }

    spin_lock(&kernel_pt_lock);

    for (; mapped < size / PAGE_SIZE; mapped += 1) {
        pt = pt_save;

        vpn[0] = (vaddr >> ADDR_0_BIT) & 0x1FFULL;
        vpn[1] = (vaddr >> ADDR_1_BIT) & 0x1FFULL;
        vpn[2] = (vaddr >> ADDR_2_BIT) & 0x1FFULL;
        ppn[0] = (paddr >> ADDR_0_BIT) & 0x1FFULL;
        ppn[1] = (paddr >> ADDR_1_BIT) & 0x1FFULL;
        ppn[2] = (paddr >> ADDR_2_BIT) & 0x3FFFFFFULL;

        for (level = 2; level >= 1; level -= 1) {
            pte = pt->entries[vpn[level]];
            if (!(pte & PAGE_VALID)) {
                /* Create a new entry. */
                new_pt = alloc_pages(1);
                if (new_pt == NULL) { goto out_unlock; }
                memset(new_pt, 0, sizeof(*new_pt));

                pte = (((u64)new_pt) >> 2) | PAGE_VALID;
                pt->entries[vpn[level]] = pte;
            }
            pt = PTE_BRANCH_TO_PT(pte);
        }

        // When we get here, table is pointing to level 0
        pte =   (ppn[2] << PTE_PPN2_BIT)
              | (ppn[1] << PTE_PPN1_BIT)
              | (ppn[0] << PTE_PPN0_BIT)
              | bits
              | PAGE_VALID;

        pt->entries[vpn[0]] = pte;

        paddr += PAGE_SIZE;
        vaddr += PAGE_SIZE;
    }

out_unlock:;
    spin_unlock(&kernel_pt_lock);

out:;
    return mapped;
}

void free_page_table(Page_Table *pt) {
    u32 e;
    u64 pte;

    for (e = 0; e < 512; e += 1) {
        pte = pt->entries[e];
        if (pte & PAGE_VALID) {
            if (PTE_IS_BRANCH(pte)) {
                free_page_table((Page_Table*)((pte << 2) & ~0xFFFUL));
            } else {
                pt->entries[e] = 0;
            }
        }
    }

    free_pages(pt);
}


u64 virt_to_phys(Page_Table *pt, u64 vaddr) {
    u64 vpn[3];
    s32 level;
    u64 pte;
    u64 ppn[3];

    vpn[0] = (vaddr >> ADDR_0_BIT) & 0x1FFULL;
    vpn[1] = (vaddr >> ADDR_1_BIT) & 0x1FFULL;
    vpn[2] = (vaddr >> ADDR_2_BIT) & 0x1FFULL;

    for (level = 2; level >= 0; level -= 1) {
        pte = pt->entries[vpn[level]];
        if (!(pte & PAGE_VALID)) { return 0; }

        if (PTE_IS_BRANCH(pte)) {
            pt = PTE_BRANCH_TO_PT(pte);
        } else {
            ppn[0] = (pte >> PTE_PPN0_BIT) & 0x1FFULL;
            ppn[1] = (pte >> PTE_PPN1_BIT) & 0x1FFULL;
            ppn[2] = (pte >> PTE_PPN2_BIT) & 0x3FFFFFFULL;

            switch (level) {
                case 0: return (ppn[2] << ADDR_2_BIT) | (ppn[1] << ADDR_1_BIT) | (ppn[0] << ADDR_0_BIT) | (vaddr & 0xFFFULL);
                case 1: return (ppn[2] << ADDR_2_BIT) | (ppn[1] << ADDR_1_BIT) | (vaddr & 0x1FFFFFULL);
                case 2: return (ppn[2] << ADDR_2_BIT) | (vaddr & 0x3FFFFFFFULL);
            }

            return 0;
        }
    }

    return 0;
}

void user_to_kernel(void *dst, const void *src, u64 len) {
    Page_Table *pt;
    u64         src_pg;
    u64         src_pg_phys;

    pt          = sched_current(sbicall(SBI_HART_ID))->page_table;
    src_pg      = (u64)ALIGN_DOWN(src, PAGE_SIZE);
    src_pg_phys = virt_to_phys(pt, src_pg);

    src = (void*)(src_pg_phys + ((u64)src - src_pg));

    memcpy(dst, src, len);
}

void kernel_to_user(void *dst, const void *src, u64 len) {
    Page_Table *pt;
    u64         dst_pg;
    u64         dst_pg_phys;

    pt          = sched_current(sbicall(SBI_HART_ID))->page_table;
    dst_pg      = (u64)ALIGN_DOWN(dst, PAGE_SIZE);
    dst_pg_phys = virt_to_phys(pt, dst_pg);

    dst = (void*)(dst_pg_phys + ((u64)dst - dst_pg));

    memcpy(dst, src, len);
}

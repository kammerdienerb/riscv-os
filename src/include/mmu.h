#ifndef __MMU_H__
#define __MMU_H__

#include "common.h"

typedef struct {
    u64 entries[512];
} Page_Table;

extern Page_Table *kernel_pt;

#define PAGE_NONE     (0)
#define PAGE_VALID    (1UL << 0)
#define PAGE_READ     (1UL << 1)
#define PAGE_WRITE    (1UL << 2)
#define PAGE_EXECUTE  (1UL << 3)
#define PAGE_USER     (1UL << 4)
#define PAGE_GLOBAL   (1UL << 5)
#define PAGE_ACCESS   (1UL << 6)
#define PAGE_DIRTY    (1UL << 7)


#define SATP_MODE_BIT    (60)
#define SATP_MODE_SV39   (8UL << SATP_MODE_BIT)
#define SATP_ASID_BIT    (44)
#define SATP_PPN_BIT     (0)
#define SATP_SET_PPN(x)  ((((u64)x) >> 12ULL) & 0xFFFFFFFFFFFULL)
#define SATP_SET_ASID(x) ((((u64)x) & 0xFFFFULL) << 44ULL)

#define KERNEL_ASID (0xFFFFUL)

void init_mmu(void);
void activate_mmu(void);
u64  mmu_map(Page_Table *pt, u64 paddr, u64 vaddr, u64 size, u64 bits);
u64  virt_to_phys(Page_Table *pt, u64 vaddr);
void user_to_kernel(void *dst, const void *src, u64 len);
void kernel_to_user(void *dst, const void *src, u64 len);

#endif

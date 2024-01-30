#include "elf.h"
#include "process.h"
#include "kprint.h"
#include "kmalloc.h"
#include "utils.h"

s64 elf_process_image(Process *proc, u8 *elf_buff) {
    Elf64_Ehdr  eheader;
    u64         image_start;
    u64         image_end;
    u64         image_base;
    u32         i;
    Elf64_Phdr  pheader;
    void       *buff;
    void       *buff_base;
    u64         phys_addr;
    u64         page_paddr;
    u64         page_vaddr;

    memcpy(&eheader, elf_buff, sizeof(eheader));

    if (eheader.e_ident[EI_MAG0] != ELFMAG0
    ||  eheader.e_ident[EI_MAG1] != ELFMAG1
    ||  eheader.e_ident[EI_MAG2] != ELFMAG2
    ||  eheader.e_ident[EI_MAG3] != ELFMAG3) {
        kprint("not an ELF file\n");
        return -1;
    }

    if (eheader.e_machine != EM_RISCV) {
        kprint("incorrect architecture\n");
        return -2;
    }

    if (eheader.e_type != ET_EXEC) {
        kprint("not an executable\n");
        return -3;
    }

    image_start = 0xFFFFFFFFFFFFFFFFULL;
    image_end   = 0;

    for (i = 0; i < eheader.e_phnum; i += 1) {
        memcpy((void*)&pheader, elf_buff + (eheader.e_phoff + (i * sizeof(pheader))),
                 sizeof(pheader));

        if (pheader.p_type != PT_LOAD
        ||  pheader.p_memsz == 0) {
            continue;
        }

        image_start = MIN(image_start, pheader.p_vaddr);
        image_end   = MAX(image_end,   pheader.p_vaddr + pheader.p_memsz);
    }

    proc->frame.sepc = eheader.e_entry;

    image_base  = image_start;
    image_start = image_start & ~(PAGE_SIZE - 1);
    image_end   = ALIGN(image_end, PAGE_SIZE);

    buff      = kmalloc_aligned(image_end - image_start, PAGE_SIZE);
    buff_base = buff + (image_start - image_base);

    proc->image = buff;

    memset(buff, 0, image_end - image_start);

    for (i = 0; i < eheader.e_phnum; i += 1) {
        memcpy((void*)&pheader, elf_buff + (eheader.e_phoff + (i * sizeof(pheader))),
                 sizeof(pheader));

        if (pheader.p_type != PT_LOAD
        ||  pheader.p_memsz == 0) {
            continue;
        }

        memcpy(buff_base + (pheader.p_vaddr - image_base), elf_buff + pheader.p_offset, pheader.p_memsz);
    }

    phys_addr  = virt_to_phys(kernel_pt, (u64)buff_base);
    page_paddr = phys_addr  & ~(PAGE_SIZE - 1);
    page_vaddr = image_base & ~(PAGE_SIZE - 1);

    mmu_map(proc->page_table,
            page_paddr, page_vaddr,
            ALIGN(image_end - page_vaddr, PAGE_SIZE),
            PAGE_USER | PAGE_READ | PAGE_WRITE | PAGE_EXECUTE);

    return 0;
}

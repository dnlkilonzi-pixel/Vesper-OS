#include "elf.h"
#include "string.h"

/* -------------------------------------------------------------------------
 * elf_load – load an ELF32 executable from a memory buffer
 * ---------------------------------------------------------------------- */
uint32_t elf_load(const void *data, uint32_t len)
{
    if (!data || len < sizeof(elf32_ehdr_t)) {
        return 0;
    }

    const elf32_ehdr_t *eh = (const elf32_ehdr_t *)data;

    /* Validate ELF magic */
    if (eh->e_ident[0] != ELF_MAGIC0 ||
        eh->e_ident[1] != ELF_MAGIC1 ||
        eh->e_ident[2] != ELF_MAGIC2 ||
        eh->e_ident[3] != ELF_MAGIC3) {
        return 0;
    }

    /* Must be 32-bit, little-endian, executable, i386 */
    if (eh->e_ident[4] != ELF_CLASS32  ||
        eh->e_ident[5] != ELF_DATA2LSB ||
        eh->e_type      != ELF_ET_EXEC  ||
        eh->e_machine   != ELF_EM_386) {
        return 0;
    }

    if (eh->e_phoff == 0 || eh->e_phnum == 0) {
        return 0;   /* no program headers */
    }

    /* Bounds-check the program-header table */
    if (eh->e_phoff + (uint32_t)eh->e_phnum * sizeof(elf32_phdr_t) > len) {
        return 0;
    }

    const uint8_t *base = (const uint8_t *)data;

    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        const elf32_phdr_t *ph =
            (const elf32_phdr_t *)(base + eh->e_phoff +
                                   (uint32_t)i * eh->e_phentsize);

        if (ph->p_type != ELF_PT_LOAD) {
            continue;
        }
        if (ph->p_filesz == 0) {
            continue;
        }

        /* Bounds-check the segment data in the file */
        if (ph->p_offset + ph->p_filesz > len) {
            return 0;
        }

        /* Copy file bytes to virtual address */
        memcpy((void *)ph->p_vaddr,
               base + ph->p_offset,
               ph->p_filesz);

        /* Zero the BSS portion of the segment (memsz > filesz) */
        if (ph->p_memsz > ph->p_filesz) {
            memset((void *)(ph->p_vaddr + ph->p_filesz),
                   0,
                   ph->p_memsz - ph->p_filesz);
        }
    }

    return eh->e_entry;
}

/* -------------------------------------------------------------------------
 * elf_load_user – load ELF segments into a user-mode page directory
 * ---------------------------------------------------------------------- */
#include "paging.h"

/* Look up the physical frame address for a virtual page in a given PD.
 * The PD and its page tables are in the identity-mapped 0-8 MB region,
 * so we can access them directly by physical address.
 * Returns the physical address of the frame, or 0 if not mapped.
 */
static uint32_t phys_of_virt(uint32_t pd_phys, uint32_t virt)
{
    const uint32_t *pd  = (const uint32_t *)pd_phys;
    uint32_t        pdi = virt >> 22u;
    uint32_t        pti = (virt >> 12u) & 0x3FFu;

    if (!(pd[pdi] & PAGE_PRESENT)) {
        return 0u;
    }
    const uint32_t *pt = (const uint32_t *)(pd[pdi] & ~0xFFFu);
    if (!(pt[pti] & PAGE_PRESENT)) {
        return 0u;
    }
    return pt[pti] & ~0xFFFu;
}

uint32_t elf_load_user(const void *data, uint32_t len, uint32_t pd_phys)
{
    if (!data || len < sizeof(elf32_ehdr_t) || !pd_phys) {
        return 0u;
    }

    const elf32_ehdr_t *eh = (const elf32_ehdr_t *)data;

    /* Validate ELF header */
    if (eh->e_ident[0] != ELF_MAGIC0 || eh->e_ident[1] != ELF_MAGIC1 ||
        eh->e_ident[2] != ELF_MAGIC2 || eh->e_ident[3] != ELF_MAGIC3) {
        return 0u;
    }
    if (eh->e_ident[4] != ELF_CLASS32  || eh->e_ident[5] != ELF_DATA2LSB ||
        eh->e_type      != ELF_ET_EXEC  || eh->e_machine   != ELF_EM_386) {
        return 0u;
    }
    if (eh->e_phoff == 0u || eh->e_phnum == 0u) {
        return 0u;
    }
    if (eh->e_phoff + (uint32_t)eh->e_phnum * sizeof(elf32_phdr_t) > len) {
        return 0u;
    }

    const uint8_t *base = (const uint8_t *)data;

    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        const elf32_phdr_t *ph =
            (const elf32_phdr_t *)(base + eh->e_phoff +
                                   (uint32_t)i * eh->e_phentsize);

        if (ph->p_type != ELF_PT_LOAD || ph->p_memsz == 0u) {
            continue;
        }
        if (ph->p_offset + ph->p_filesz > len) {
            return 0u;
        }

        /* Page-align the virtual range */
        uint32_t page_start = ph->p_vaddr & ~0xFFFu;
        uint32_t page_end   = (ph->p_vaddr + ph->p_memsz + 0xFFFu) & ~0xFFFu;
        uint32_t n_pages    = (page_end - page_start) >> 12u;

        /* Allocate frames and map them in pd_phys with PAGE_USER */
        if (paging_alloc_user_pages(pd_phys, page_start, n_pages) != 0) {
            return 0u;
        }

        /* Copy or zero-fill each page via the physical frame address */
        for (uint32_t pg = 0u; pg < n_pages; pg++) {
            uint32_t virt_page = page_start + pg * 4096u;
            uint32_t phys_page = phys_of_virt(pd_phys, virt_page);
            if (!phys_page) {
                return 0u;
            }

            uint8_t *dst = (uint8_t *)phys_page;   /* identity-mapped */
            memset(dst, 0, 4096u);

            /* Determine the overlap of this page with the file-data region */
            uint32_t file_start = ph->p_vaddr;
            uint32_t file_end   = ph->p_vaddr + ph->p_filesz;
            uint32_t pg_end     = virt_page + 4096u;

            if (virt_page >= file_end || pg_end <= file_start) {
                continue;   /* page is entirely BSS – already zeroed */
            }

            uint32_t copy_vstart = (virt_page > file_start)
                                   ? virt_page : file_start;
            uint32_t copy_vend   = (pg_end < file_end) ? pg_end : file_end;

            uint32_t dst_off = copy_vstart - virt_page;
            uint32_t src_off = ph->p_offset + (copy_vstart - ph->p_vaddr);
            uint32_t copy_len = copy_vend - copy_vstart;

            memcpy(dst + dst_off, base + src_off, copy_len);
        }
    }

    return eh->e_entry;
}

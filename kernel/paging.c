#include "paging.h"
#include "string.h"

/* -------------------------------------------------------------------------
 * Static paging structures
 *
 * These must be 4 KB-aligned.  Placing them in BSS (zero-initialised by
 * kernel_entry.asm before kernel_main is called) is safe.
 *
 * Two page tables cover the first 8 MB:
 *   page_tables[0] : 0x00000000 – 0x003FFFFF  (4 MB)
 *   page_tables[1] : 0x00400000 – 0x007FFFFF  (4 MB)
 * ---------------------------------------------------------------------- */
#define PT_ENTRIES  1024u
#define PD_ENTRIES  1024u
#define PAGE_SIZE   4096u

static uint32_t page_directory[PD_ENTRIES]     __attribute__((aligned(4096)));
static uint32_t page_tables[2][PT_ENTRIES]     __attribute__((aligned(4096)));

void paging_init(void)
{
    /* Clear page directory (all entries not-present) */
    memset(page_directory, 0, sizeof(page_directory));

    /* Build identity-map page tables for the first 8 MB */
    for (uint32_t t = 0; t < 2u; t++) {
        for (uint32_t i = 0; i < PT_ENTRIES; i++) {
            uint32_t phys = t * (4u * 1024u * 1024u) + i * PAGE_SIZE;
            page_tables[t][i] = phys | PAGE_PRESENT | PAGE_WRITABLE;
        }
        /* Point the PDE at our static page table */
        page_directory[t] = (uint32_t)page_tables[t] | PAGE_PRESENT | PAGE_WRITABLE;
    }

    /* Load CR3 with the physical address of the page directory.
     * (virtual == physical here because paging is not yet enabled) */
    __asm__ volatile ("mov %0, %%cr3" : : "r"(page_directory) : "memory");

    /* Enable paging: set CR0.PG (bit 31) */
    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

void paging_map_page(uint32_t virt, uint32_t phys, uint32_t flags)
{
    uint32_t pdi = virt >> 22u;
    uint32_t pti = (virt >> 12u) & 0x3FFu;

    if (!(page_directory[pdi] & PAGE_PRESENT)) {
        return;   /* no page table allocated for this 4 MB region */
    }

    uint32_t *pt = (uint32_t *)(page_directory[pdi] & ~0xFFFu);
    pt[pti] = (phys & ~0xFFFu) | PAGE_PRESENT | (flags & 0xFFEu);
    paging_flush_tlb(virt);
}

void paging_flush_tlb(uint32_t virt)
{
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

uint32_t paging_get_cr3(void)
{
    uint32_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

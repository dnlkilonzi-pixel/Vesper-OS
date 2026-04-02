#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

/*
 * VESPER OS – Paging (x86 32-bit, two-level page tables)
 *
 * paging_init() sets up an identity-mapped page directory that covers the
 * first 8 MB of physical RAM (two 4 MB page tables), then enables paging
 * by loading CR3 and setting CR0.PG.
 *
 * Because virtual == physical for the first 8 MB, the kernel, stack, VGA
 * buffer (0xB8000), and hardware I/O-mapped regions all work unchanged.
 *
 * paging_map_page() can map additional single pages once paging is active,
 * provided the destination PDE already has a page table allocated.
 */

/* Page-table entry flags (bits 0-2) */
#define PAGE_PRESENT   (1u << 0)
#define PAGE_WRITABLE  (1u << 1)
#define PAGE_USER      (1u << 2)   /* accessible from ring 3 */

/*
 * Initialise paging:
 *   – Identity-map physical 0x00000000 – 0x007FFFFF (8 MB)
 *   – Load CR3, enable CR0.PG
 */
void paging_init(void);

/*
 * Map a single 4 KB virtual page to a physical frame.
 * Silently ignored if the PDE for that virtual address has no page table.
 * @flags : combination of PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
 */
void paging_map_page(uint32_t virt, uint32_t phys, uint32_t flags);

/* Invalidate a single TLB entry */
void paging_flush_tlb(uint32_t virt);

/* Return the physical address of the active page directory (for new processes) */
uint32_t paging_get_cr3(void);

#endif /* PAGING_H */

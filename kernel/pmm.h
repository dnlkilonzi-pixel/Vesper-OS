#ifndef PMM_H
#define PMM_H

#include <stdint.h>

/*
 * VESPER OS – Physical Memory Manager
 *
 * Bitmap-based frame allocator.  Each bit represents one 4 KB physical
 * page frame (0 = free, 1 = used).  The bitmap covers up to 32 MB of RAM
 * which is the amount we give QEMU.
 *
 * The bootloader stores an E820 memory map at BOOT_MMAP_ADDR before entering
 * protected mode.  pmm_init() reads that map and marks usable frames free.
 */

#define PMM_PAGE_SIZE    4096u
#define PMM_PAGE_SHIFT   12u

/* Physical address where the bootloader writes the E820 map */
#define BOOT_MMAP_ADDR   0x0500u

/* Maximum physical memory tracked (32 MB) */
#define PMM_MAX_PHYS     (32u * 1024u * 1024u)
#define PMM_TOTAL_FRAMES (PMM_MAX_PHYS / PMM_PAGE_SIZE)   /* 8192 */

/* E820 memory-map entry (20-byte BIOS format) */
typedef struct {
    uint64_t base;      /* region start address     */
    uint64_t length;    /* region length in bytes   */
    uint32_t type;      /* 1 = usable RAM           */
} __attribute__((packed)) e820_entry_t;

/* Boot memory-map header stored at BOOT_MMAP_ADDR */
typedef struct {
    uint16_t     count;          /* number of valid entries (0 if E820 failed) */
    e820_entry_t entries[32];    /* up to 32 entries                           */
} __attribute__((packed)) boot_mmap_t;

#define E820_TYPE_USABLE  1u

/* Initialise allocator from the E820 map (or a safe default if absent) */
void     pmm_init(void);

/*
 * Allocate one physical 4 KB frame.
 * Returns the physical base address, or 0 if out of memory.
 */
uint32_t pmm_alloc_frame(void);

/* Release a previously allocated frame */
void     pmm_free_frame(uint32_t phys_addr);

/* Diagnostic counters */
uint32_t pmm_free_frames(void);
uint32_t pmm_total_frames(void);

#endif /* PMM_H */

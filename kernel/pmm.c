#include "pmm.h"
#include "string.h"

/* -------------------------------------------------------------------------
 * Bitmap storage – one bit per 4 KB frame, covering PMM_MAX_PHYS bytes.
 * bit = 0 → free frame,  bit = 1 → used frame.
 * ---------------------------------------------------------------------- */
static uint8_t  pmm_bitmap[PMM_TOTAL_FRAMES / 8u];   /* 1024 bytes */
static uint32_t pmm_free  = 0;
static uint32_t pmm_total = 0;

/* ---- bit-manipulation helpers ----------------------------------------- */

static inline void pmm_set_used(uint32_t frame)
{
    pmm_bitmap[frame >> 3u] |= (uint8_t)(1u << (frame & 7u));
}

static inline void pmm_set_free(uint32_t frame)
{
    pmm_bitmap[frame >> 3u] &= (uint8_t)~(1u << (frame & 7u));
}

static inline int pmm_is_used(uint32_t frame)
{
    return (pmm_bitmap[frame >> 3u] >> (frame & 7u)) & 1u;
}

/* -------------------------------------------------------------------------
 * pmm_reserve_range – mark [base, base+len) as used
 * ---------------------------------------------------------------------- */
static void pmm_reserve_range(uint64_t base, uint64_t len)
{
    if (base >= PMM_MAX_PHYS) {
        return;
    }
    if (base + len > PMM_MAX_PHYS) {
        len = PMM_MAX_PHYS - base;
    }

    uint32_t start = (uint32_t)(base / PMM_PAGE_SIZE);
    uint32_t end   = (uint32_t)((base + len + PMM_PAGE_SIZE - 1u) / PMM_PAGE_SIZE);
    if (end > PMM_TOTAL_FRAMES) {
        end = PMM_TOTAL_FRAMES;
    }

    for (uint32_t f = start; f < end; f++) {
        if (!pmm_is_used(f)) {
            pmm_set_used(f);
            if (pmm_free > 0) {
                pmm_free--;
            }
        }
    }
}

/* -------------------------------------------------------------------------
 * pmm_init – populate the bitmap from the E820 map stored at 0x0500
 * ---------------------------------------------------------------------- */
void pmm_init(void)
{
    /* Start with everything marked as used */
    memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));
    pmm_free  = 0;
    pmm_total = 0;

    /*
     * Read the E820 memory map stored by the bootloader at 0x0500.
     * GCC's -Warray-bounds fires a false positive here because 0x0500 is a
     * small constant integer that its analyser assumes is "near zero" (i.e.
     * likely NULL + small offset).  In a bare-metal kernel 0x0500 is a
     * perfectly valid physical address; the pragma silences the false
     * positive for this entire E820-reading block.
     */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"

    const boot_mmap_t *mmap =
        (const boot_mmap_t *)(uintptr_t)BOOT_MMAP_ADDR;

    if (mmap->count == 0) {
        /*
         * E820 unavailable – apply a conservative fixed layout:
         *   0x00000000 – 0x000FFFFF : reserved (BIOS, kernel)
         *   0x00100000 – 0x01FFFFFF : usable   (1 MB – 32 MB)
         */
        uint32_t start = 0x100000u / PMM_PAGE_SIZE;   /* frame 256 */
        uint32_t end   = PMM_TOTAL_FRAMES;             /* frame 8192 */

        for (uint32_t f = start; f < end; f++) {
            pmm_set_free(f);
            pmm_free++;
            pmm_total++;
        }
        /*
         * Count the first 0x100000/PAGE_SIZE reserved frames toward
         * pmm_total so that free% statistics are accurate.  These
         * frames remain set (used) in the bitmap.
         */
        pmm_total += 0x100000u / PMM_PAGE_SIZE;
        return;
    }

    /* Process each E820 entry */
    for (uint16_t i = 0; i < mmap->count; i++) {
        const e820_entry_t *e = &mmap->entries[i];

        if (e->type != E820_TYPE_USABLE) {
            continue;
        }
        if (e->base >= PMM_MAX_PHYS) {
            continue;
        }

        uint64_t base = e->base;
        uint64_t end  = base + e->length;
        if (end > PMM_MAX_PHYS) {
            end = PMM_MAX_PHYS;
        }

        /* Align start up, end down to page boundaries */
        uint32_t start_frame = (uint32_t)((base + PMM_PAGE_SIZE - 1u) / PMM_PAGE_SIZE);
        uint32_t end_frame   = (uint32_t)(end / PMM_PAGE_SIZE);

        if (end_frame > PMM_TOTAL_FRAMES) {
            end_frame = PMM_TOTAL_FRAMES;
        }

        for (uint32_t f = start_frame; f < end_frame; f++) {
            pmm_set_free(f);
            pmm_free++;
        }
        pmm_total += end_frame - start_frame;
    }

#pragma GCC diagnostic pop

    /*
     * Re-mark the first 1 MB of physical memory as reserved regardless of
     * what the E820 map said.  This protects the BIOS IVT (0x0000), the
     * E820 map itself (0x0500), the boot sector (0x7C00), the kernel binary
     * (starting at 0x1000), and the kernel stack (below 0x90000).
     */
    pmm_reserve_range(0x0u, 0x100000u);   /* first 1 MB = 0x100000/PAGE_SIZE frames */
}

/* -------------------------------------------------------------------------
 * pmm_alloc_frame – first-fit scan of the bitmap
 * ---------------------------------------------------------------------- */
uint32_t pmm_alloc_frame(void)
{
    for (uint32_t f = 0; f < PMM_TOTAL_FRAMES; f++) {
        if (!pmm_is_used(f)) {
            pmm_set_used(f);
            if (pmm_free > 0) {
                pmm_free--;
            }
            return f * PMM_PAGE_SIZE;
        }
    }
    return 0u;   /* out of memory */
}

/* -------------------------------------------------------------------------
 * pmm_free_frame – return a frame to the pool
 * ---------------------------------------------------------------------- */
void pmm_free_frame(uint32_t phys_addr)
{
    uint32_t f = phys_addr / PMM_PAGE_SIZE;
    if (f >= PMM_TOTAL_FRAMES) {
        return;
    }
    if (pmm_is_used(f)) {
        pmm_set_free(f);
        pmm_free++;
    }
}

uint32_t pmm_free_frames(void)  { return pmm_free;  }
uint32_t pmm_total_frames(void) { return pmm_total; }

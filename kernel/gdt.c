#include "gdt.h"
#include "tss.h"
#include <stdint.h>

/* -------------------------------------------------------------------------
 * GDT entry (8-byte gate / segment descriptor)
 * ---------------------------------------------------------------------- */
typedef struct {
    uint16_t limit_low;         /* limit bits  0-15 */
    uint16_t base_low;          /* base  bits  0-15 */
    uint8_t  base_mid;          /* base  bits 16-23 */
    uint8_t  access;            /* access byte      */
    uint8_t  flags_limit_high;  /* flags[7:4] | limit[3:0] */
    uint8_t  base_high;         /* base  bits 24-31 */
} __attribute__((packed)) gdt_entry_t;

/* GDT pointer (6 bytes) loaded with LGDT */
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

/* Six-entry table:  null / kcode / kdata / ucode / udata / TSS */
static gdt_entry_t gdt_table[6];
static gdt_ptr_t   gdt_ptr;

/* -------------------------------------------------------------------------
 * gdt_set_entry – fill one descriptor
 *
 * @limit : 20-bit limit (granularity chosen by flags)
 * @access: P|DPL|S|Type  (e.g. 0x9A for kernel code)
 * @flags : high nibble = G|D/B|0|0,  low nibble OR'd with limit[19:16]
 * ---------------------------------------------------------------------- */
static void gdt_set_entry(int idx, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t flags)
{
    gdt_table[idx].base_low         = (uint16_t)(base & 0xFFFFu);
    gdt_table[idx].base_mid         = (uint8_t)((base >> 16u) & 0xFFu);
    gdt_table[idx].base_high        = (uint8_t)((base >> 24u) & 0xFFu);
    gdt_table[idx].limit_low        = (uint16_t)(limit & 0xFFFFu);
    gdt_table[idx].flags_limit_high = (uint8_t)(((limit >> 16u) & 0x0Fu) |
                                                 (flags & 0xF0u));
    gdt_table[idx].access           = access;
}

/* -------------------------------------------------------------------------
 * gdt_init – build the table, load it, flush CS with a far jump
 * ---------------------------------------------------------------------- */
void gdt_init(void)
{
    /* 0: Null descriptor – required, all zeros */
    gdt_set_entry(0, 0, 0, 0x00, 0x00);

    /* 1: Kernel code – base 0, limit 4 GB, DPL=0, execute/read
     *    access: P=1 DPL=00 S=1 Type=1010 → 0x9A
     *    flags : G=1 D/B=1                → high nibble = 0xC  */
    gdt_set_entry(1, 0, 0xFFFFFu, 0x9Au, 0xCFu);

    /* 2: Kernel data – base 0, limit 4 GB, DPL=0, read/write
     *    access: P=1 DPL=00 S=1 Type=0010 → 0x92 */
    gdt_set_entry(2, 0, 0xFFFFFu, 0x92u, 0xCFu);

    /* 3: User code – same as kernel code but DPL=3
     *    access: P=1 DPL=11 S=1 Type=1010 → 0xFA */
    gdt_set_entry(3, 0, 0xFFFFFu, 0xFAu, 0xCFu);

    /* 4: User data – same as kernel data but DPL=3
     *    access: P=1 DPL=11 S=1 Type=0010 → 0xF2 */
    gdt_set_entry(4, 0, 0xFFFFFu, 0xF2u, 0xCFu);

    /* 5: TSS descriptor (32-bit TSS, available)
     *    access: P=1 DPL=0 S=0 Type=1001 → 0x89
     *    flags : G=0 D/B=0               → 0x00 (byte granularity) */
    gdt_set_entry(5,
                  tss_get_base(),
                  tss_get_limit(),
                  0x89u, 0x00u);

    gdt_ptr.limit = (uint16_t)(sizeof(gdt_table) - 1u);
    gdt_ptr.base  = (uint32_t)gdt_table;

    /*
     * Load new GDT and reload all segment registers.
     * CS cannot be changed with a simple MOV; a far jump is required.
     * The ljmp flushes the instruction prefetch queue and reloads CS.
     */
    __asm__ volatile (
        "lgdt (%0)          \n"
        "mov  $0x10, %%ax   \n"   /* kernel data selector */
        "mov  %%ax,  %%ds   \n"
        "mov  %%ax,  %%es   \n"
        "mov  %%ax,  %%fs   \n"
        "mov  %%ax,  %%gs   \n"
        "mov  %%ax,  %%ss   \n"
        "ljmp $0x08, $1f    \n"   /* far jump → CS = kernel code selector */
        "1:                 \n"
        :
        : "r"(&gdt_ptr)
        : "ax", "memory"
    );
}

#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/*
 * VESPER OS – Global Descriptor Table (expanded)
 *
 * The bootloader's minimal GDT (null / kernel-code / kernel-data) is
 * replaced at boot time by this six-entry table loaded from C:
 *
 *   Index 0 – selector 0x00 : null descriptor (required)
 *   Index 1 – selector 0x08 : kernel code  (DPL 0, execute/read, 4 GB flat)
 *   Index 2 – selector 0x10 : kernel data  (DPL 0, read/write,   4 GB flat)
 *   Index 3 – selector 0x18 : user   code  (DPL 3, execute/read, 4 GB flat)
 *   Index 4 – selector 0x20 : user   data  (DPL 3, read/write,   4 GB flat)
 *   Index 5 – selector 0x28 : TSS          (32-bit TSS available)
 *
 * User-mode code must use (0x18 | 3) = 0x1B for CS and (0x20 | 3) = 0x23
 * for the data/stack segments so the RPL bits match the CPL.
 */

/* Kernel segment selectors */
#define GDT_SEL_NULL   0x00u
#define GDT_SEL_KCODE  0x08u
#define GDT_SEL_KDATA  0x10u

/* User segment selectors (add RPL=3 when loading from ring 3) */
#define GDT_SEL_UCODE  0x18u
#define GDT_SEL_UDATA  0x20u

/* TSS descriptor selector */
#define GDT_SEL_TSS    0x28u

/* Ring-3 selectors with RPL=3 set */
#define GDT_USER_CS    (GDT_SEL_UCODE | 3u)   /* 0x1B */
#define GDT_USER_DS    (GDT_SEL_UDATA | 3u)   /* 0x23 */

/*
 * gdt_init – build and install the expanded GDT, reload all segment
 *            registers, and (via tss.c) load TR.
 *
 * Must be called after tss_init() because it reads the TSS base/limit.
 */
void gdt_init(void);

#endif /* GDT_H */

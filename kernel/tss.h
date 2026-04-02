#ifndef TSS_H
#define TSS_H

#include <stdint.h>

/*
 * VESPER OS – Task State Segment (TSS)
 *
 * The TSS is required for privilege-level changes.  When the CPU switches
 * from ring 3 to ring 0 (e.g. INT 0x80 syscall or hardware IRQ while a
 * user task is running), it reads ESP0 and SS0 from the TSS to set up the
 * kernel stack before pushing the interrupt frame.
 *
 * We use a single TSS shared by all kernel threads; tss_set_kernel_stack()
 * updates ESP0 before entering user mode so that each process gets its own
 * kernel stack on the next ring-0 transition.
 */

typedef struct {
    uint32_t prev_tss;    /* selector of previous TSS when using hardware task-switch */
    uint32_t esp0;        /* ring-0 stack pointer (loaded on privilege change)        */
    uint32_t ss0;         /* ring-0 stack segment                                     */
    uint32_t esp1;        /* unused – rings 1/2 not used                              */
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;  /* offset from TSS base to I/O permission bitmap */
} __attribute__((packed)) tss_t;

/*
 * tss_init – zero the TSS and set SS0 / ESP0.
 * @kernel_esp : initial ring-0 stack pointer (kernel stack top)
 */
void tss_init(uint32_t kernel_esp);

/*
 * tss_flush – execute LTR to load the task register with the TSS selector.
 * Must be called AFTER gdt_init() has installed the TSS descriptor.
 */
void tss_flush(void);

/* Update ESP0 so the next ring-3 → ring-0 transition uses this stack */
void tss_set_kernel_stack(uint32_t esp);

/* Called by gdt_init() to obtain the TSS physical address and byte size */
uint32_t tss_get_base(void);
uint32_t tss_get_limit(void);

#endif /* TSS_H */

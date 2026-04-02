#include "tss.h"
#include "gdt.h"
#include "string.h"

static tss_t tss;

void tss_init(uint32_t kernel_esp)
{
    memset(&tss, 0, sizeof(tss));
    tss.ss0        = GDT_SEL_KDATA;   /* 0x10 – kernel data segment      */
    tss.esp0       = kernel_esp;
    tss.iomap_base = (uint16_t)sizeof(tss_t);   /* no I/O permission bitmap */
}

void tss_flush(void)
{
    /* Load the task register with the TSS selector (GDT index 5 = 0x28) */
    __asm__ volatile ("ltr %%ax" : : "a"((uint16_t)GDT_SEL_TSS));
}

void tss_set_kernel_stack(uint32_t esp)
{
    tss.esp0 = esp;
}

uint32_t tss_get_base(void)  { return (uint32_t)&tss;       }
uint32_t tss_get_limit(void) { return sizeof(tss_t) - 1u;   }

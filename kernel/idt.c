#include "idt.h"
#include "isr.h"

/* -------------------------------------------------------------------------
 * Forward declarations of the ASM stubs in isr.asm
 * ---------------------------------------------------------------------- */

/* CPU exception stubs (vectors 0-19) */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void);

/* Hardware IRQ stubs (IRQ 0-15 → vectors 32-47) */
extern void irq0(void);  extern void irq1(void);  extern void irq2(void);
extern void irq3(void);  extern void irq4(void);  extern void irq5(void);
extern void irq6(void);  extern void irq7(void);  extern void irq8(void);
extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void);
extern void irq15(void);

/* -------------------------------------------------------------------------
 * IDT storage
 * ---------------------------------------------------------------------- */

/* The 256-entry IDT table (256 × 8 bytes = 2 KB) */
static idt_entry_t idt_table[IDT_ENTRIES];

/* 6-byte descriptor loaded into the IDTR register */
static idt_ptr_t   idt_ptr;

/* -------------------------------------------------------------------------
 * idt_set_gate – Populate one IDT slot
 * ---------------------------------------------------------------------- */
void idt_set_gate(uint8_t vector, uint32_t handler,
                  uint16_t selector, uint8_t type_attr)
{
    idt_table[vector].offset_low  = (uint16_t)(handler & 0xFFFF);
    idt_table[vector].selector    = selector;
    idt_table[vector].zero        = 0;
    idt_table[vector].type_attr   = type_attr;
    idt_table[vector].offset_high = (uint16_t)((handler >> 16) & 0xFFFF);
}

/* -------------------------------------------------------------------------
 * idt_init – Wire all gates and load the IDT
 * ---------------------------------------------------------------------- */
void idt_init(void)
{
    /* Build the IDT descriptor */
    idt_ptr.limit = (uint16_t)(sizeof(idt_table) - 1);
    idt_ptr.base  = (uint32_t)idt_table;

    /* CPU exception handlers (vectors 0-19)
     * Kernel code segment selector = 0x08, interrupt gate = 0x8E */
#define GATE(n) idt_set_gate(n, (uint32_t)isr##n, 0x08, IDT_INTERRUPT_GATE)
    GATE(0);  GATE(1);  GATE(2);  GATE(3);  GATE(4);
    GATE(5);  GATE(6);  GATE(7);  GATE(8);  GATE(9);
    GATE(10); GATE(11); GATE(12); GATE(13); GATE(14);
    GATE(15); GATE(16); GATE(17); GATE(18); GATE(19);
#undef GATE

    /* Hardware IRQ handlers (vectors 32-47, remapped from IRQ 0-15) */
#define IRQGATE(n) idt_set_gate(32 + n, (uint32_t)irq##n, 0x08, IDT_INTERRUPT_GATE)
    IRQGATE(0);  IRQGATE(1);  IRQGATE(2);  IRQGATE(3);
    IRQGATE(4);  IRQGATE(5);  IRQGATE(6);  IRQGATE(7);
    IRQGATE(8);  IRQGATE(9);  IRQGATE(10); IRQGATE(11);
    IRQGATE(12); IRQGATE(13); IRQGATE(14); IRQGATE(15);
#undef IRQGATE

    /* Load the new IDT into the processor's IDTR register */
    __asm__ volatile ("lidt (%0)" : : "r"(&idt_ptr));
}

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/*
 * VESPER OS – Interrupt Descriptor Table (IDT)
 *
 * The IDT tells the CPU where to find the handler for each of the 256
 * possible interrupt vectors.  Each entry is an 8-byte "gate descriptor".
 *
 * We use 32-bit interrupt gates (type 0xE) for all handlers:
 *   – Interrupt gates automatically clear IF (disables further interrupts)
 *     on entry, preventing handler re-entrancy.
 *
 * Gate descriptor layout (32-bit interrupt gate):
 *
 *   Bits  0-15  : offset[15:0]  – low 16 bits of handler address
 *   Bits 16-31  : selector      – code segment selector (= 0x08)
 *   Bits 32-39  : zero          – reserved, must be 0
 *   Bits 40-47  : type_attr     – 0x8E (P=1, DPL=0, S=0, type=1110)
 *   Bits 48-63  : offset[31:16] – high 16 bits of handler address
 */

/* Number of IDT entries (CPU supports 0–255) */
#define IDT_ENTRIES 256

/* 32-bit interrupt-gate attribute byte:
 *   Bit 7    (P)    = 1  : present
 *   Bits 6-5 (DPL)  = 00 : ring 0
 *   Bit 4    (S)    = 0  : system gate
 *   Bits 3-0 (Type) = 1110 = 0xE : 32-bit interrupt gate
 */
#define IDT_INTERRUPT_GATE 0x8E

/* idt_entry_t – one 8-byte IDT gate descriptor */
typedef struct {
    uint16_t offset_low;    /* Handler address bits  0-15 */
    uint16_t selector;      /* Kernel code segment selector */
    uint8_t  zero;          /* Reserved, always 0 */
    uint8_t  type_attr;     /* Gate type and attributes */
    uint16_t offset_high;   /* Handler address bits 16-31 */
} __attribute__((packed)) idt_entry_t;

/* idt_ptr_t – 6-byte structure loaded with the LIDT instruction */
typedef struct {
    uint16_t limit;   /* Size of IDT in bytes minus 1 */
    uint32_t base;    /* Linear address of the IDT */
} __attribute__((packed)) idt_ptr_t;

/*
 * idt_set_gate – Fill one IDT slot.
 *
 * @vector    : Interrupt vector number (0–255)
 * @handler   : Address of the ASM stub that handles this vector
 * @selector  : Code segment selector (0x08 for kernel code)
 * @type_attr : Gate type / attributes (use IDT_INTERRUPT_GATE)
 */
void idt_set_gate(uint8_t vector, uint32_t handler,
                  uint16_t selector, uint8_t type_attr);

/*
 * idt_init – Populate all IDT gates and load the IDT with LIDT.
 *            Call this before enabling hardware interrupts.
 */
void idt_init(void);

#endif /* IDT_H */

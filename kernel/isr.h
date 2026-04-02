#ifndef ISR_H
#define ISR_H

#include <stdint.h>

/*
 * VESPER OS – Interrupt Service Routine (ISR) / IRQ C-level dispatcher
 *
 * The ASM stubs in isr.asm build a uniform register-save frame on the stack
 * and call interrupt_handler(registers_t *), which is defined here.
 *
 * Stack layout when interrupt_handler() is called (32-bit cdecl):
 *
 *   [regs+0 ]  gs
 *   [regs+4 ]  fs
 *   [regs+8 ]  es
 *   [regs+12]  ds
 *   [regs+16]  edi  ─┐
 *   [regs+20]  esi   │  saved by PUSHA
 *   [regs+24]  ebp   │
 *   [regs+28]  old_esp (ESP value before PUSHA)
 *   [regs+32]  ebx   │
 *   [regs+36]  edx   │
 *   [regs+40]  ecx   │
 *   [regs+44]  eax  ─┘
 *   [regs+48]  int_no   – pushed by the stub
 *   [regs+52]  err_code – pushed by stub (dummy 0) or by CPU
 *   [regs+56]  eip    ─┐
 *   [regs+60]  cs      │  pushed by CPU on interrupt
 *   [regs+64]  eflags ─┘
 *
 * (For ring-0 → ring-0 transitions the CPU does NOT push SS/ESP.)
 */

/* Saved CPU state passed to every C interrupt handler */
typedef struct registers {
    uint32_t gs, fs, es, ds;                        /* segment regs  */
    uint32_t edi, esi, ebp, old_esp,                /* from PUSHA    */
             ebx, edx, ecx, eax;
    uint32_t int_no, err_code;                      /* stub-pushed   */
    uint32_t eip, cs, eflags;                       /* CPU-pushed    */
} registers_t;

/* Prototype for a C-level interrupt / IRQ handler */
typedef void (*irq_handler_t)(registers_t *);

/*
 * irq_register_handler – Register a C function to handle the given IRQ.
 *
 * @irq     : 0–15, where IRQ 1 = PS/2 keyboard, IRQ 0 = PIT timer, etc.
 * @handler : Function to call when the IRQ fires.  NULL to deregister.
 */
void irq_register_handler(uint8_t irq, irq_handler_t handler);

/*
 * interrupt_handler – Single C dispatcher called by the common ASM stub.
 *   – Vectors  0–19 : CPU exceptions (print diagnostics, halt).
 *   – Vectors 32–47 : Hardware IRQs (dispatch to registered handler, send EOI).
 */
void interrupt_handler(registers_t *regs);

#endif /* ISR_H */

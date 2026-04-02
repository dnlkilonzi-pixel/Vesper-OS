#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* PS/2 controller I/O ports */
#define KEYBOARD_DATA_PORT   0x60   /* Read: scancode / Write: command */
#define KEYBOARD_STATUS_PORT 0x64   /* Bit 0 set = output buffer full  */

/*
 * keyboard_init – Set up the interrupt-driven keyboard driver.
 *
 * Registers the PS/2 keyboard IRQ handler (IRQ 1) and unmasks IRQ 1 on the
 * PIC.  Must be called after idt_init() and pic_init().
 * STI must be called after keyboard_init() for interrupts to actually fire.
 */
void keyboard_init(void);

/*
 * keyboard_getchar – Block until a printable character is available.
 *
 * Uses HLT to yield the CPU while the ring buffer is empty, so no busy
 * spinning.  Returns the ASCII code of the key pressed (including Enter
 * '\n' and Backspace '\b').
 */
char keyboard_getchar(void);

#endif /* KEYBOARD_H */

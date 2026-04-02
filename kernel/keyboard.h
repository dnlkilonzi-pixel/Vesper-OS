#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* PS/2 controller I/O ports */
#define KEYBOARD_DATA_PORT   0x60   /* Read: scancode / Write: command */
#define KEYBOARD_STATUS_PORT 0x64   /* Bit 0 set = output buffer full  */

/*
 * Special key codes (> 0x7F so they cannot be confused with ASCII).
 * These are placed in the ring buffer by the IRQ handler and can be
 * read by keyboard_getchar().
 */
#define KEY_UP    0x80   /* ↑ arrow */
#define KEY_DOWN  0x81   /* ↓ arrow */
#define KEY_LEFT  0x82   /* ← arrow */
#define KEY_RIGHT 0x83   /* → arrow */

/*
 * keyboard_init – Set up the interrupt-driven keyboard driver.
 */
void keyboard_init(void);

/*
 * keyboard_getchar – Block until a key is available.
 * Returns the ASCII code (for printable keys) or one of the KEY_* constants
 * above for special keys.
 */
char keyboard_getchar(void);

#endif /* KEYBOARD_H */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* PS/2 controller I/O ports */
#define KEYBOARD_DATA_PORT   0x60   /* Read: scancode / Write: command */
#define KEYBOARD_STATUS_PORT 0x64   /* Bit 0 set = output buffer full  */

void keyboard_init(void);

/* Block until a printable character (or Enter/Backspace) is available,
 * then return the corresponding ASCII code. */
char keyboard_getchar(void);

#endif /* KEYBOARD_H */

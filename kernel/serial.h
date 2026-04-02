#ifndef SERIAL_H
#define SERIAL_H

/*
 * VESPER OS – Serial console driver (COM1, 115200 8N1)
 *
 * Output is mirrored to the COM1 UART so QEMU's "-serial stdio" captures
 * kernel log lines without a VGA window.
 */

/* Initialise COM1 at 115200 baud, 8N1, FIFOs enabled */
void serial_init(void);

/* Transmit one character (blocks until TX holding register is empty) */
void serial_putc(char c);

/* Transmit a NUL-terminated string ('\n' → "\r\n") */
void serial_puts(const char *s);

#endif /* SERIAL_H */

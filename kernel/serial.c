#include "serial.h"
#include "port_io.h"

/* COM1 I/O base and register offsets */
#define COM1_BASE   0x3F8u

#define COM1_DATA   (COM1_BASE + 0u)   /* DLAB=0: TX/RX data          */
#define COM1_IER    (COM1_BASE + 1u)   /* DLAB=0: interrupt enable    */
#define COM1_FCR    (COM1_BASE + 2u)   /* FIFO control                */
#define COM1_LCR    (COM1_BASE + 3u)   /* line control (bit7 = DLAB)  */
#define COM1_MCR    (COM1_BASE + 4u)   /* modem control               */
#define COM1_LSR    (COM1_BASE + 5u)   /* line status                 */

/* DLAB-mode registers (accessed when LCR bit 7 = 1) */
#define COM1_DLL    (COM1_BASE + 0u)   /* divisor latch low           */
#define COM1_DLH    (COM1_BASE + 1u)   /* divisor latch high          */

/* Line Status Register bits */
#define LSR_TX_EMPTY  0x20u            /* TX holding register empty   */

/*
 * Baud-rate divisor for 115200 baud:
 *   divisor = 115200 / 115200 = 1
 */
#define BAUD_DIVISOR  1u

void serial_init(void)
{
    outb(COM1_IER, 0x00);   /* disable all UART interrupts             */

    /* Set baud-rate divisor (DLAB=1 to access DLL/DLH) */
    outb(COM1_LCR, 0x80);   /* enable Divisor Latch Access Bit         */
    outb(COM1_DLL, (uint8_t)(BAUD_DIVISOR & 0xFFu));
    outb(COM1_DLH, (uint8_t)(BAUD_DIVISOR >> 8u));

    /* 8 data bits, no parity, 1 stop bit (DLAB=0) */
    outb(COM1_LCR, 0x03);

    /* Enable and reset FIFOs, 14-byte threshold */
    outb(COM1_FCR, 0xC7);

    /* Enable RTS, DTR, OUT2 (OUT2 gates IRQ on real hardware; harmless in QEMU) */
    outb(COM1_MCR, 0x0B);
}

static int serial_tx_ready(void)
{
    return (inb(COM1_LSR) & LSR_TX_EMPTY) != 0;
}

void serial_putc(char c)
{
    while (!serial_tx_ready()) {
        /* spin until TX holding register is empty */
    }
    outb(COM1_DATA, (uint8_t)c);
}

void serial_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            serial_putc('\r');  /* convert LF → CRLF for terminal emulators */
        }
        serial_putc(*s++);
    }
}

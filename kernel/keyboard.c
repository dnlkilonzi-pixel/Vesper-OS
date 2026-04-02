#include "keyboard.h"

/* -------------------------------------------------------------------------
 * Low-level port I/O
 * ---------------------------------------------------------------------- */

/* Read one byte from the given I/O port */
static inline uint8_t inb(uint16_t port)
{
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/* Spin until the PS/2 output buffer is non-empty (bit 0 of status port) */
static void keyboard_wait(void)
{
    while (!(inb(KEYBOARD_STATUS_PORT) & 0x01))
        ;
}

/* -------------------------------------------------------------------------
 * Scancode → ASCII translation tables (PS/2 Scancode Set 1)
 *
 * Index = scancode value (make code).  Value 0 = no printable mapping.
 * Covers scancodes 0x00–0x39 (Space key).
 * ---------------------------------------------------------------------- */
static const char scancode_ascii[] = {
    /* 0x00 */ 0,
    /* 0x01 */ 0,     /* Escape */
    /* 0x02 */ '1',
    /* 0x03 */ '2',
    /* 0x04 */ '3',
    /* 0x05 */ '4',
    /* 0x06 */ '5',
    /* 0x07 */ '6',
    /* 0x08 */ '7',
    /* 0x09 */ '8',
    /* 0x0A */ '9',
    /* 0x0B */ '0',
    /* 0x0C */ '-',
    /* 0x0D */ '=',
    /* 0x0E */ '\b',  /* Backspace */
    /* 0x0F */ '\t',  /* Tab */
    /* 0x10 */ 'q',
    /* 0x11 */ 'w',
    /* 0x12 */ 'e',
    /* 0x13 */ 'r',
    /* 0x14 */ 't',
    /* 0x15 */ 'y',
    /* 0x16 */ 'u',
    /* 0x17 */ 'i',
    /* 0x18 */ 'o',
    /* 0x19 */ 'p',
    /* 0x1A */ '[',
    /* 0x1B */ ']',
    /* 0x1C */ '\n',  /* Enter */
    /* 0x1D */ 0,     /* Left Ctrl */
    /* 0x1E */ 'a',
    /* 0x1F */ 's',
    /* 0x20 */ 'd',
    /* 0x21 */ 'f',
    /* 0x22 */ 'g',
    /* 0x23 */ 'h',
    /* 0x24 */ 'j',
    /* 0x25 */ 'k',
    /* 0x26 */ 'l',
    /* 0x27 */ ';',
    /* 0x28 */ '\'',
    /* 0x29 */ '`',
    /* 0x2A */ 0,     /* Left Shift */
    /* 0x2B */ '\\',
    /* 0x2C */ 'z',
    /* 0x2D */ 'x',
    /* 0x2E */ 'c',
    /* 0x2F */ 'v',
    /* 0x30 */ 'b',
    /* 0x31 */ 'n',
    /* 0x32 */ 'm',
    /* 0x33 */ ',',
    /* 0x34 */ '.',
    /* 0x35 */ '/',
    /* 0x36 */ 0,     /* Right Shift */
    /* 0x37 */ '*',   /* Keypad * */
    /* 0x38 */ 0,     /* Left Alt */
    /* 0x39 */ ' ',   /* Space */
};

/* Same table with Shift held */
static const char scancode_ascii_shift[] = {
    /* 0x00 */ 0,
    /* 0x01 */ 0,
    /* 0x02 */ '!',
    /* 0x03 */ '@',
    /* 0x04 */ '#',
    /* 0x05 */ '$',
    /* 0x06 */ '%',
    /* 0x07 */ '^',
    /* 0x08 */ '&',
    /* 0x09 */ '*',
    /* 0x0A */ '(',
    /* 0x0B */ ')',
    /* 0x0C */ '_',
    /* 0x0D */ '+',
    /* 0x0E */ '\b',
    /* 0x0F */ '\t',
    /* 0x10 */ 'Q',
    /* 0x11 */ 'W',
    /* 0x12 */ 'E',
    /* 0x13 */ 'R',
    /* 0x14 */ 'T',
    /* 0x15 */ 'Y',
    /* 0x16 */ 'U',
    /* 0x17 */ 'I',
    /* 0x18 */ 'O',
    /* 0x19 */ 'P',
    /* 0x1A */ '{',
    /* 0x1B */ '}',
    /* 0x1C */ '\n',
    /* 0x1D */ 0,
    /* 0x1E */ 'A',
    /* 0x1F */ 'S',
    /* 0x20 */ 'D',
    /* 0x21 */ 'F',
    /* 0x22 */ 'G',
    /* 0x23 */ 'H',
    /* 0x24 */ 'J',
    /* 0x25 */ 'K',
    /* 0x26 */ 'L',
    /* 0x27 */ ':',
    /* 0x28 */ '"',
    /* 0x29 */ '~',
    /* 0x2A */ 0,
    /* 0x2B */ '|',
    /* 0x2C */ 'Z',
    /* 0x2D */ 'X',
    /* 0x2E */ 'C',
    /* 0x2F */ 'V',
    /* 0x30 */ 'B',
    /* 0x31 */ 'N',
    /* 0x32 */ 'M',
    /* 0x33 */ '<',
    /* 0x34 */ '>',
    /* 0x35 */ '?',
    /* 0x36 */ 0,
    /* 0x37 */ '*',
    /* 0x38 */ 0,
    /* 0x39 */ ' ',
};

/* -------------------------------------------------------------------------
 * Driver state
 * ---------------------------------------------------------------------- */

static int shift_pressed = 0;   /* Non-zero while a Shift key is held */

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

void keyboard_init(void)
{
    shift_pressed = 0;
}

/* Block until a printable ASCII character is available and return it */
char keyboard_getchar(void)
{
    while (1) {
        keyboard_wait();
        uint8_t scancode = inb(KEYBOARD_DATA_PORT);

        /* Break codes (key-release) have bit 7 set */
        if (scancode & 0x80) {
            uint8_t released = scancode & 0x7F;
            /* Track Shift release (left=0x2A, right=0x36) */
            if (released == 0x2A || released == 0x36) {
                shift_pressed = 0;
            }
            continue;
        }

        /* Track Shift press */
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 1;
            continue;
        }

        /* Map scancode to ASCII */
        if (scancode < sizeof(scancode_ascii)) {
            char c = shift_pressed
                     ? scancode_ascii_shift[scancode]
                     : scancode_ascii[scancode];
            if (c != 0) {
                return c;
            }
        }
    }
}

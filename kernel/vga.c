#include "vga.h"
#include "port_io.h"

/* VGA CRTC index/data ports used to reposition the hardware cursor */
#define VGA_CRTC_IDX  0x3D4
#define VGA_CRTC_DATA 0x3D5

/* VGA text-mode buffer (volatile: writes must reach hardware every time) */
static volatile uint16_t * const vga_buffer =
    (volatile uint16_t *)VGA_ADDRESS;

/* Software cursor position */
static int vga_row   = 0;
static int vga_col   = 0;

/* Current colour attribute byte (fg | bg<<4) */
static uint8_t vga_color = 0;

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/* Pack a character and colour into a VGA cell word */
static inline uint16_t vga_entry(char c, uint8_t color)
{
    /* Cast through unsigned char to avoid sign-extension of high-ASCII */
    return (uint16_t)(unsigned char)c | ((uint16_t)color << 8);
}

/* Build an attribute byte from foreground and background colour indices */
static inline uint8_t make_color(vga_color_t fg, vga_color_t bg)
{
    return (uint8_t)fg | ((uint8_t)bg << 4);
}

/* Move the VGA controller's hardware cursor to (vga_row, vga_col).
 *
 * The 6845/VGA CRTC exposes the cursor position through two 8-bit registers
 * accessed via an index/data pair:
 *   Reg 0x0E – cursor position high byte
 *   Reg 0x0F – cursor position low byte
 *
 * The position is a linear offset: row * VGA_COLS + col.
 */
static void vga_update_cursor(void)
{
    uint16_t pos = (uint16_t)(vga_row * VGA_COLS + vga_col);
    outb(VGA_CRTC_IDX,  0x0E);
    outb(VGA_CRTC_DATA, (uint8_t)((pos >> 8) & 0xFF));
    outb(VGA_CRTC_IDX,  0x0F);
    outb(VGA_CRTC_DATA, (uint8_t)(pos & 0xFF));
}

/* Scroll the entire screen up by one row and clear the last line */
static void vga_scroll(void)
{
    /* Move every row one position upward */
    for (int row = 0; row < VGA_ROWS - 1; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            vga_buffer[row * VGA_COLS + col] =
                vga_buffer[(row + 1) * VGA_COLS + col];
        }
    }

    /* Blank the bottom row */
    for (int col = 0; col < VGA_COLS; col++) {
        vga_buffer[(VGA_ROWS - 1) * VGA_COLS + col] =
            vga_entry(' ', vga_color);
    }

    vga_row = VGA_ROWS - 1;
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

/* Set the active text colour */
void vga_set_color(vga_color_t fg, vga_color_t bg)
{
    vga_color = make_color(fg, bg);
}

/* Initialise the VGA driver */
void vga_init(void)
{
    vga_row   = 0;
    vga_col   = 0;
    vga_color = make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
}

/* Erase the screen and reset the cursor */
void vga_clear(void)
{
    for (int row = 0; row < VGA_ROWS; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            vga_buffer[row * VGA_COLS + col] = vga_entry(' ', vga_color);
        }
    }
    vga_row = 0;
    vga_col = 0;
    vga_update_cursor();
}

/* Write one character to the current cursor position, advancing as needed */
void vga_putchar(char c)
{
    if (c == '\n') {
        /* Newline: move to start of next row */
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        /* Carriage return: move to start of current row */
        vga_col = 0;
    } else if (c == '\b') {
        /* Backspace: erase the preceding character */
        if (vga_col > 0) {
            vga_col--;
            vga_buffer[vga_row * VGA_COLS + vga_col] =
                vga_entry(' ', vga_color);
        }
    } else {
        /* Printable character: place in buffer then advance */
        vga_buffer[vga_row * VGA_COLS + vga_col] = vga_entry(c, vga_color);
        vga_col++;
    }

    /* Wrap to the next row if we hit the right edge */
    if (vga_col >= VGA_COLS) {
        vga_col = 0;
        vga_row++;
    }

    /* Scroll if the cursor moved past the last row */
    if (vga_row >= VGA_ROWS) {
        vga_scroll();
    }

    vga_update_cursor();
}

/* Write a NUL-terminated string */
void vga_puts(const char *str)
{
    while (*str) {
        vga_putchar(*str++);
    }
}

/* Reposition the software cursor (bounds-checked) */
void vga_set_cursor(int row, int col)
{
    if (row >= 0 && row < VGA_ROWS && col >= 0 && col < VGA_COLS) {
        vga_row = row;
        vga_col = col;
        vga_update_cursor();
    }
}

int vga_get_row(void) { return vga_row; }
int vga_get_col(void) { return vga_col; }

/* Print an unsigned 32-bit integer in decimal */
void vga_print_uint(uint32_t n)
{
    if (n == 0) {
        vga_putchar('0');
        return;
    }

    /* Build digits in reverse, then print forward */
    char buf[12];   /* 2^32 = 4294967296, 10 digits + NUL + spare */
    int  i = 0;

    while (n > 0) {
        buf[i++] = (char)('0' + (n % 10));
        n /= 10;
    }

    /* Reverse-print */
    for (int j = i - 1; j >= 0; j--) {
        vga_putchar(buf[j]);
    }
}

/* Print an unsigned 32-bit integer as "0xXXXXXXXX" */
void vga_print_hex(uint32_t n)
{
    vga_puts("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        uint32_t nibble = (n >> shift) & 0xFu;
        vga_putchar(nibble < 10u
                    ? (char)('0' + nibble)
                    : (char)('A' + nibble - 10u));
    }
}

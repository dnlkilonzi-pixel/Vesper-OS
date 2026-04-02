#ifndef VGA_H
#define VGA_H

#include <stdint.h>

/* VGA text-mode buffer is mapped at physical address 0xB8000.
 * Each cell is a 16-bit word:
 *   bits  7:0  – ASCII character code
 *   bits 11:8  – foreground colour (4-bit index)
 *   bits 15:12 – background colour (4-bit index)
 */
#define VGA_ADDRESS 0xB8000

/* Standard 80×25 text-mode dimensions */
#define VGA_COLS 80
#define VGA_ROWS 25

/* 16-colour VGA palette indices */
typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW        = 14,
    VGA_COLOR_WHITE         = 15,
} vga_color_t;

/* Initialise the VGA driver, clear screen, reset cursor to (0,0) */
void vga_init(void);

/* Erase all characters and reset cursor to (0,0) */
void vga_clear(void);

/* Write a single character (handles \n, \r, \b) */
void vga_putchar(char c);

/* Write a NUL-terminated string */
void vga_puts(const char *str);

/* Change active text colour */
void vga_set_color(vga_color_t fg, vga_color_t bg);

/* Move the software cursor */
void vga_set_cursor(int row, int col);

/* Query current cursor row */
int vga_get_row(void);

/* Query current cursor column */
int vga_get_col(void);

/* Print an unsigned 32-bit integer in decimal */
void vga_print_uint(uint32_t n);

/* Print an unsigned 32-bit integer as "0xXXXXXXXX" (uppercase hex) */
void vga_print_hex(uint32_t n);

#endif /* VGA_H */

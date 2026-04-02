#include "kernel.h"
#include "vga.h"
#include "shell.h"

/* -------------------------------------------------------------------------
 * kernel_main – called by kernel_entry.asm immediately after the bootloader
 *               hands over control in 32-bit protected mode.
 * ---------------------------------------------------------------------- */
void kernel_main(void)
{
    /* Initialise VGA text mode (clears screen, resets cursor & colour) */
    vga_init();

    /* ---- Banner -------------------------------------------------------- */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("================================================================\n");
    vga_puts("              VESPER KERNEL ACTIVE                             \n");
    vga_puts("          Minimal Educational OS  v0.1.0                       \n");
    vga_puts("================================================================\n");

    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("\n");
    vga_puts("  Architecture : x86  32-bit Protected Mode\n");
    vga_puts("  VGA buffer   : 0xB8000\n");
    vga_puts("  Kernel base  : 0x1000\n");
    vga_puts("  Stack base   : 0x90000\n");
    vga_puts("\n");
    vga_puts("  Type 'help' for available commands.\n");
    vga_puts("\n");

    /* ---- Shell --------------------------------------------------------- */
    shell_init();
    shell_run();   /* Never returns */

    /* Safety net: halt if shell_run() somehow returns */
    while (1) {
        __asm__ volatile ("hlt");
    }
}

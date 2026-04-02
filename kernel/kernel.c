#include "kernel.h"
#include "vga.h"
#include "shell.h"
#include "kmem.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"

/* -------------------------------------------------------------------------
 * kernel_main – called by kernel_entry.asm immediately after the bootloader
 *               hands over control in 32-bit protected mode.
 *
 * Initialisation order:
 *   1. vga_init()       – screen must come first (diagnostic output)
 *   2. kmem_init()      – heap allocator
 *   3. idt_init()       – load IDT (but interrupts still disabled)
 *   4. pic_init()       – remap 8259A PIC, mask all IRQs
 *   5. keyboard_init()  – register IRQ1 handler, unmask IRQ1
 *   6. sti              – enable hardware interrupts
 *   7. shell_run()      – interactive shell (never returns)
 * ---------------------------------------------------------------------- */
void kernel_main(void)
{
    /* ---- Step 1: VGA -------------------------------------------------- */
    vga_init();

    /* ---- Banner ------------------------------------------------------- */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("================================================================\n");
    vga_puts("              VESPER KERNEL ACTIVE                             \n");
    vga_puts("          Minimal Educational OS  v0.2.0                       \n");
    vga_puts("================================================================\n");

    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("\n");
    vga_puts("  Architecture : x86  32-bit Protected Mode\n");
    vga_puts("  VGA buffer   : 0xB8000\n");
    vga_puts("  Kernel base  : 0x1000\n");
    vga_puts("  Stack base   : 0x90000\n");
    vga_puts("  Heap size    : 256 KB\n");
    vga_puts("\n");

    /* ---- Step 2: Heap ------------------------------------------------- */
    kmem_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Heap allocator  (256 KB)\n");

    /* ---- Step 3: IDT -------------------------------------------------- */
    idt_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] IDT loaded      (256 gates)\n");

    /* ---- Step 4: PIC -------------------------------------------------- */
    pic_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] PIC remapped    (IRQ0-15 -> INT 32-47)\n");

    /* ---- Step 5: Keyboard --------------------------------------------- */
    keyboard_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Keyboard driver (IRQ1, interrupt-driven)\n");

    /* ---- Step 6: Enable interrupts ------------------------------------ */
    __asm__ volatile ("sti");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Interrupts enabled\n");

    /* ---- Step 7: Shell ------------------------------------------------ */
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("\n  Type 'help' for available commands.\n\n");

    shell_run();   /* Never returns */

    /* Safety net */
    while (1) {
        __asm__ volatile ("hlt");
    }
}

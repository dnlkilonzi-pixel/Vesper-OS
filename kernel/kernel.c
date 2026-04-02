#include "kernel.h"
#include "vga.h"
#include "serial.h"
#include "shell.h"
#include "kmem.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "timer.h"
#include "pmm.h"
#include "paging.h"
#include "gdt.h"
#include "tss.h"
#include "process.h"
#include "syscall.h"
#include "ata.h"
#include "fs.h"

/* -------------------------------------------------------------------------
 * kernel_main – called by kernel_entry.asm immediately after BSS is zeroed.
 *
 * Initialisation order:
 *   1.  vga_init()      – screen must come first (diagnostic output)
 *   2.  serial_init()   – mirror output to COM1 (QEMU -serial stdio)
 *   3.  kmem_init()     – static heap allocator
 *   4.  tss_init()      – zero TSS, set initial kernel stack
 *   5.  gdt_init()      – load expanded GDT (user segs + TSS descriptor)
 *   6.  tss_flush()     – execute LTR (requires GDT to be loaded first)
 *   7.  idt_init()      – load IDT (interrupts still disabled)
 *   8.  pic_init()      – remap 8259A PIC, mask all IRQs
 *   9.  timer_init()    – program PIT at 100 Hz, unmask IRQ0
 *   10. keyboard_init() – register IRQ1 handler, unmask IRQ1
 *   11. pmm_init()      – physical frame allocator (reads E820 map)
 *   12. paging_init()   – identity-map 0–8 MB, enable CR0.PG
 *   13. syscall_init()  – install INT 0x80 gate (DPL=3)
 *   14. process_init()  – create idle process, initialise scheduler
 *   15. ata_init()      – detect primary ATA drive
 *   16. fs_init()       – check/report VesperFS status
 *   17. sti             – enable hardware interrupts
 *   18. shell_run()     – interactive shell (never returns)
 * ---------------------------------------------------------------------- */
void kernel_main(void)
{
    /* ---- Step 1: VGA -------------------------------------------------- */
    vga_init();

    /* ---- Banner ------------------------------------------------------- */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("================================================================\n");
    vga_puts("              VESPER KERNEL ACTIVE                             \n");
    vga_puts("          Full-featured Educational OS  v0.3.0                 \n");
    vga_puts("================================================================\n");

    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("\n");
    vga_puts("  Architecture : x86  32-bit Protected Mode\n");
    vga_puts("  VGA buffer   : 0xB8000\n");
    vga_puts("  Kernel base  : 0x1000\n");
    vga_puts("  Stack base   : 0x90000\n");
    vga_puts("  Heap size    : 256 KB\n");
    vga_puts("\n");

    /* ---- Step 2: Serial ----------------------------------------------- */
    serial_init();
    serial_puts("VESPER: serial console active (115200 8N1)\n");

    /* ---- Step 3: Heap ------------------------------------------------- */
    kmem_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Heap allocator  (256 KB)\n");

    /* ---- Steps 4-6: GDT + TSS ----------------------------------------- */
    tss_init(0x90000u);   /* initial kernel stack top (matches bootloader) */
    gdt_init();
    tss_flush();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] GDT loaded      (6 entries: null/kcode/kdata/ucode/udata/TSS)\n");
    vga_puts("  [OK] TSS installed   (ESP0=0x90000, SS0=0x10)\n");

    /* ---- Step 7: IDT -------------------------------------------------- */
    idt_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] IDT loaded      (256 gates)\n");

    /* ---- Step 8: PIC -------------------------------------------------- */
    pic_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] PIC remapped    (IRQ0-15 -> INT 32-47)\n");

    /* ---- Step 9: Timer ------------------------------------------------ */
    timer_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] PIT timer       (IRQ0, 100 Hz)\n");

    /* ---- Step 10: Keyboard -------------------------------------------- */
    keyboard_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Keyboard driver (IRQ1, interrupt-driven)\n");

    /* ---- Step 11: PMM ------------------------------------------------- */
    pmm_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] PMM ready       (");
    vga_print_uint(pmm_free_frames() * 4u);
    vga_puts(" KB free)\n");

    /* ---- Step 12: Paging ---------------------------------------------- */
    paging_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Paging enabled  (identity-map 0-8 MB)\n");

    /* ---- Step 13: Syscalls -------------------------------------------- */
    syscall_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Syscall gate    (INT 0x80, DPL=3)\n");

    /* ---- Step 14: Processes ------------------------------------------- */
    process_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Scheduler       (co-operative, max ");
    vga_print_uint(MAX_PROCESSES);
    vga_puts(" processes)\n");

    /* ---- Step 15: ATA ------------------------------------------------- */
    if (ata_init()) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_puts("  [OK] ATA drive       (primary master, PIO)\n");
    } else {
        vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
        vga_puts("  [--] ATA drive       (not detected)\n");
    }

    /* ---- Step 16: Filesystem ------------------------------------------ */
    if (fs_init()) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_puts("  [OK] VesperFS        (mounted at LBA 129)\n");
    } else {
        vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
        vga_puts("  [--] VesperFS        (not formatted – run 'mkfs')\n");
    }

    /* ---- Step 17: Enable interrupts ----------------------------------- */
    __asm__ volatile ("sti");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Interrupts enabled\n");

    /* ---- Step 18: Shell ----------------------------------------------- */
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("\n  Type 'help' for available commands.\n\n");

    shell_run();   /* Never returns */

    /* Safety net */
    while (1) {
        __asm__ volatile ("hlt");
    }
}

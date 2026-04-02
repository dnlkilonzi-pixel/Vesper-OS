#include "kernel.h"
#include "vga.h"
#include "serial.h"
#include "shell.h"
#include "kmem.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "timer.h"
#include "rtc.h"
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
 * Initialisation order (Tier 2):
 *   1.  vga_init()      – screen must come first
 *   2.  serial_init()   – COM1 115200 baud
 *   3.  kmem_init()     – static heap
 *   4.  tss_init()      – zero TSS, set initial ESP0
 *   5.  gdt_init()      – load GDT (null/kcode/kdata/ucode/udata/TSS)
 *   6.  tss_flush()     – LTR (requires GDT loaded)
 *   7.  idt_init()      – load IDT
 *   8.  pic_init()      – remap 8259A, mask all
 *   9.  timer_init()    – PIT 100 Hz, preemptive slice = 50 ms
 *   10. keyboard_init() – IRQ1, extended scancodes (arrow keys)
 *   11. pmm_init()      – bitmap PMM (reads E820)
 *   12. paging_init()   – identity-map 0–8 MB, enable CR0.PG
 *   13. syscall_init()  – INT 0x80 gate (DPL=3)
 *   14. process_init()  – idle process, preemptive scheduler
 *   15. ata_init()      – ATA PIO primary master
 *   16. fs_init()       – VesperFS check
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
    vga_puts("        True-Multitasking Educational OS  v0.4.0               \n");
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
    tss_init(0x90000u);
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
    vga_printf("  [OK] PIT timer       (IRQ0, 100 Hz, preempt every %u ticks)\n",
               (uint32_t)SCHED_SLICE);

    /* ---- Step 10: Keyboard -------------------------------------------- */
    keyboard_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Keyboard driver (IRQ1, extended scancodes)\n");

    /* ---- Step 11: PMM ------------------------------------------------- */
    pmm_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] PMM ready       (");
    vga_print_uint(pmm_free_frames() * 4u);
    vga_puts(" KB free)\n");

    /* ---- Step 12: Paging ---------------------------------------------- */
    paging_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Paging enabled  (identity-map 0-8 MB, per-process PDs)\n");

    /* ---- Step 13: Syscalls -------------------------------------------- */
    syscall_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Syscall gate    (INT 0x80, DPL=3)\n");

    /* ---- Step 14: Processes ------------------------------------------- */
    process_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_printf("  [OK] Scheduler       (preemptive round-robin, max %u procs)\n",
               (uint32_t)MAX_PROCESSES);

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

    while (1) {
        __asm__ volatile ("hlt");
    }
}

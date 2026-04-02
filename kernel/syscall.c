#include "syscall.h"
#include "idt.h"
#include "vga.h"
#include "timer.h"
#include "process.h"

/* IDT gate attribute for a DPL=3 interrupt gate (callable from ring 3) */
#define IDT_SYSCALL_GATE  0xEEu   /* P=1 DPL=3 S=0 Type=1110 (32-bit int gate) */

/* Forward declaration of the ASM stub defined in isr.asm */
extern void isr_syscall(void);

void syscall_init(void)
{
    idt_set_gate(0x80u, (uint32_t)isr_syscall, 0x08u, IDT_SYSCALL_GATE);
}

/* -------------------------------------------------------------------------
 * syscall_handler – dispatched from the INT 0x80 stub in isr.asm
 * ---------------------------------------------------------------------- */
void syscall_handler(uint32_t eax, uint32_t ebx, uint32_t ecx)
{
    switch (eax) {

    case SYS_EXIT:
        /* Terminate the calling process */
        process_exit();
        break;

    case SYS_WRITE:
        /*
         * Write ECX bytes from the buffer at EBX to stdout (VGA).
         * We trust the address for now (no address-space isolation yet).
         */
        {
            const char *buf = (const char *)(uintptr_t)ebx;
            uint32_t    len = ecx;
            for (uint32_t i = 0; i < len; i++) {
                vga_putchar(buf[i]);
            }
        }
        break;

    case SYS_GETPID:
        /* Return value goes back via EAX; caller reads it after INT 0x80 */
        /* (The stub would need to write back to the saved-register frame,
         *  which we keep simple for now by just printing a note.) */
        break;

    case SYS_SLEEP:
        timer_sleep_ms(ebx);
        break;

    default:
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("[syscall] unknown syscall ");
        vga_print_uint(eax);
        vga_putchar('\n');
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        break;
    }
}

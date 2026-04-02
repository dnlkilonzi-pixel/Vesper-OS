#include "isr.h"
#include "pic.h"
#include "vga.h"

/* NULL for a freestanding environment */
#define NULL ((void *)0)

/* -------------------------------------------------------------------------
 * IRQ handler registration table
 *
 * Slot irq_handlers[n] holds the C handler for IRQ n (0-15), or NULL if
 * no driver has registered for that line.
 * ---------------------------------------------------------------------- */
static irq_handler_t irq_handlers[16];

void irq_register_handler(uint8_t irq, irq_handler_t handler)
{
    if (irq < 16) {
        irq_handlers[irq] = handler;
    }
}

/* -------------------------------------------------------------------------
 * Human-readable names for the first 20 CPU exception vectors
 * ---------------------------------------------------------------------- */
static const char * const exception_names[] = {
    "#DE Divide-by-Zero",          /*  0 */
    "#DB Debug",                   /*  1 */
    "Non-Maskable Interrupt",      /*  2 */
    "#BP Breakpoint",              /*  3 */
    "#OF Overflow",                /*  4 */
    "#BR Bound Range Exceeded",    /*  5 */
    "#UD Invalid Opcode",          /*  6 */
    "#NM Device Not Available",    /*  7 */
    "#DF Double Fault",            /*  8 */
    "Coprocessor Segment Overrun", /*  9 */
    "#TS Invalid TSS",             /* 10 */
    "#NP Segment Not Present",     /* 11 */
    "#SS Stack-Segment Fault",     /* 12 */
    "#GP General Protection Fault",/* 13 */
    "#PF Page Fault",              /* 14 */
    "Reserved",                    /* 15 */
    "#MF x87 FPU Error",           /* 16 */
    "#AC Alignment Check",         /* 17 */
    "#MC Machine Check",           /* 18 */
    "#XM SIMD FP Exception",       /* 19 */
};

/* -------------------------------------------------------------------------
 * isr_handler – Called for CPU exceptions (vectors 0-19)
 * ---------------------------------------------------------------------- */
static void isr_handler(registers_t *regs)
{
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_puts("\n*** KERNEL EXCEPTION ***\n");
    vga_puts("  Vector   : ");
    vga_print_uint(regs->int_no);
    vga_puts("\n  Name     : ");

    if (regs->int_no < 20) {
        vga_puts(exception_names[regs->int_no]);
    } else {
        vga_puts("(reserved)");
    }

    vga_puts("\n  EIP      : ");
    vga_print_hex(regs->eip);
    vga_puts("\n  Error    : ");
    vga_print_hex(regs->err_code);
    vga_puts("\n  EFLAGS   : ");
    vga_print_hex(regs->eflags);
    vga_puts("\n\nSystem halted.\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Halt – exceptions are not recoverable in our minimal kernel */
    __asm__ volatile ("cli; hlt");
    while (1) {}
}

/* -------------------------------------------------------------------------
 * irq_handler – Called for hardware IRQs (vectors 32-47)
 * ---------------------------------------------------------------------- */
static void irq_handler(registers_t *regs)
{
    uint8_t irq = (uint8_t)(regs->int_no - 32);

    /* Dispatch to the registered driver handler (if any) */
    if (irq < 16 && irq_handlers[irq] != NULL) {
        irq_handlers[irq](regs);
    }

    /* Acknowledge the PIC so it can raise future interrupts */
    pic_send_eoi(irq);
}

/* -------------------------------------------------------------------------
 * interrupt_handler – Single C entry point called by common_stub in isr.asm
 * ---------------------------------------------------------------------- */
void interrupt_handler(registers_t *regs)
{
    if (regs->int_no < 32) {
        /* CPU exception */
        isr_handler(regs);
    } else {
        /* Hardware IRQ */
        irq_handler(regs);
    }
}

#include "timer.h"
#include "isr.h"
#include "pic.h"
#include "port_io.h"
#include "process.h"

/* PIT I/O ports */
#define PIT_CHANNEL0  0x40u
#define PIT_CMD       0x43u

/*
 * PIT input frequency: 1,193,182 Hz
 * Command byte 0x34:
 *   bits 7-6 : 00   → channel 0
 *   bits 5-4 : 11   → low byte + high byte
 *   bits 3-1 : 010  → mode 2 (rate generator)
 *   bit  0   : 0    → binary counting
 */
#define PIT_FREQ      1193182u
#define PIT_CMD_BYTE  0x34u
#define PIT_DIVISOR   (PIT_FREQ / TIMER_HZ)

static volatile uint32_t ticks = 0;

/* -------------------------------------------------------------------------
 * timer_irq_handler – called every 10 ms (TIMER_HZ = 100)
 *
 * Preemptive scheduling:
 *   Every SCHED_SLICE ticks (50 ms) we send the PIC EOI early (so the PIC
 *   can accept new IRQs) and then call process_yield().  process_yield
 *   saves the current kernel-stack frame, picks the next process, and
 *   returns in the new process's context.  When control eventually returns
 *   here it means the original process has been re-scheduled; execution
 *   continues normally up the call chain through irq_handler → common_stub
 *   → IRET back to wherever the process was running before the IRQ fired.
 * ---------------------------------------------------------------------- */
static void timer_irq_handler(registers_t *regs)
{
    (void)regs;
    ticks++;

    if ((ticks % SCHED_SLICE) == 0u && current_process) {
        /*
         * Send EOI *before* process_yield so the PIC can accept new IRQs
         * while we are inside the scheduler.  The irq_handler caller will
         * send EOI again after we return, which is harmless.
         */
        pic_send_eoi(0u);
        process_yield();
    }
}

void timer_init(void)
{
    ticks = 0;

    outb(PIT_CMD,      PIT_CMD_BYTE);
    outb(PIT_CHANNEL0, (uint8_t)(PIT_DIVISOR & 0xFFu));
    outb(PIT_CHANNEL0, (uint8_t)((PIT_DIVISOR >> 8u) & 0xFFu));

    irq_register_handler(0, timer_irq_handler);
    pic_unmask_irq(0);
}

uint32_t timer_get_ticks(void)
{
    return ticks;
}

uint32_t timer_get_uptime_sec(void)
{
    return ticks / TIMER_HZ;
}

void timer_sleep_ms(uint32_t ms)
{
    uint32_t target = ticks + ((ms * TIMER_HZ + 999u) / 1000u);
    while (ticks < target) {
        __asm__ volatile ("hlt");
    }
}

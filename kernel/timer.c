#include "timer.h"
#include "isr.h"
#include "pic.h"
#include "port_io.h"

/* PIT I/O ports */
#define PIT_CHANNEL0  0x40u   /* channel 0 data port            */
#define PIT_CMD       0x43u   /* mode/command register           */

/*
 * PIT input frequency: 1,193,182 Hz (derived from 14.31818 MHz / 12)
 *
 * Command byte 0x34:
 *   bits 7-6 : 00   → channel 0
 *   bits 5-4 : 11   → access mode: low byte then high byte
 *   bits 3-1 : 010  → mode 2 (rate generator – one pulse per period)
 *   bit  0   : 0    → binary counting
 */
#define PIT_FREQ      1193182u
#define PIT_CMD_BYTE  0x34u
#define PIT_DIVISOR   (PIT_FREQ / TIMER_HZ)   /* 11931 for 100 Hz */

static volatile uint32_t ticks = 0;

/* IRQ0 handler: increment the global tick counter */
static void timer_irq_handler(registers_t *regs)
{
    (void)regs;
    ticks++;
}

void timer_init(void)
{
    ticks = 0;

    /* Program PIT channel 0 */
    outb(PIT_CMD,      PIT_CMD_BYTE);
    outb(PIT_CHANNEL0, (uint8_t)(PIT_DIVISOR & 0xFFu));
    outb(PIT_CHANNEL0, (uint8_t)((PIT_DIVISOR >> 8u) & 0xFFu));

    /* Register handler and unmask IRQ0 on the master PIC */
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

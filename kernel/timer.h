#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/*
 * VESPER OS – PIT (8253/8254) timer driver
 *
 * Programs PIT channel 0 to fire IRQ0 at TIMER_HZ times per second.
 * The tick counter is incremented by the IRQ0 handler.
 */

#define TIMER_HZ  100u   /* 100 Hz → 10 ms per tick */

/* Initialise the PIT and register the IRQ0 handler */
void timer_init(void);

/* Return the raw tick count (incremented TIMER_HZ times per second) */
uint32_t timer_get_ticks(void);

/* Return seconds since timer_init() was called */
uint32_t timer_get_uptime_sec(void);

/* Busy-wait (HLT loop) for the given number of milliseconds */
void timer_sleep_ms(uint32_t ms);

#endif /* TIMER_H */

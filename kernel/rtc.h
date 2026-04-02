#ifndef RTC_H
#define RTC_H

#include <stdint.h>

/*
 * VESPER OS – CMOS Real-Time Clock driver
 *
 * Reads the current date and time from the CMOS RTC via I/O ports
 * 0x70 (index) and 0x71 (data).
 *
 * The driver handles both BCD and binary-mode CMOS chips, and applies
 * a Y2K two-digit year correction (year < 70 → 2000+, else 1900+).
 */

typedef struct {
    uint16_t year;    /* full year e.g. 2025           */
    uint8_t  month;   /* 1–12                          */
    uint8_t  day;     /* 1–31                          */
    uint8_t  hour;    /* 0–23                          */
    uint8_t  minute;  /* 0–59                          */
    uint8_t  second;  /* 0–59                          */
} rtc_time_t;

/*
 * rtc_read – read the current time from the CMOS RTC.
 * Spins until the "update-in-progress" flag is clear before reading
 * to avoid a torn read across a one-second boundary.
 */
void rtc_read(rtc_time_t *t);

#endif /* RTC_H */

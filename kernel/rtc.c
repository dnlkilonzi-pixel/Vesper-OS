#include "rtc.h"
#include "port_io.h"

/* CMOS I/O ports */
#define CMOS_IDX   0x70u   /* index register (write + NMI disable bit) */
#define CMOS_DATA  0x71u   /* data register  (read)                    */

/* CMOS register addresses */
#define RTC_REG_SECOND  0x00u
#define RTC_REG_MINUTE  0x02u
#define RTC_REG_HOUR    0x04u
#define RTC_REG_DAY     0x07u
#define RTC_REG_MONTH   0x08u
#define RTC_REG_YEAR    0x09u
#define RTC_REG_STATUS_A 0x0Au  /* bit 7 = update-in-progress           */
#define RTC_REG_STATUS_B 0x0Bu  /* bit 2 = binary mode, bit 1 = 24h     */

/* Bit 7 in the index register disables NMI when set (safe practice) */
#define NMI_DISABLE  0x80u

/* -------------------------------------------------------------------------
 * cmos_read – select a CMOS register and read its value
 * ---------------------------------------------------------------------- */
static uint8_t cmos_read(uint8_t reg)
{
    outb(CMOS_IDX, NMI_DISABLE | reg);
    return inb(CMOS_DATA);
}

/* -------------------------------------------------------------------------
 * bcd_to_bin – convert a packed BCD byte to binary
 * ---------------------------------------------------------------------- */
static uint8_t bcd_to_bin(uint8_t bcd)
{
    return (uint8_t)(((bcd >> 4u) & 0x0Fu) * 10u + (bcd & 0x0Fu));
}

/* -------------------------------------------------------------------------
 * rtc_read – read current date and time from CMOS
 * ---------------------------------------------------------------------- */
void rtc_read(rtc_time_t *t)
{
    /* Wait until the "update-in-progress" flag is clear */
    while (cmos_read(RTC_REG_STATUS_A) & 0x80u) {
        /* spin */
    }

    uint8_t status_b = cmos_read(RTC_REG_STATUS_B);

    t->second = cmos_read(RTC_REG_SECOND);
    t->minute = cmos_read(RTC_REG_MINUTE);
    t->hour   = cmos_read(RTC_REG_HOUR);
    t->day    = cmos_read(RTC_REG_DAY);
    t->month  = cmos_read(RTC_REG_MONTH);
    t->year   = cmos_read(RTC_REG_YEAR);

    /* Convert BCD to binary if bit 2 of Status B is NOT set */
    if (!(status_b & 0x04u)) {
        t->second = bcd_to_bin(t->second);
        t->minute = bcd_to_bin(t->minute);
        t->hour   = bcd_to_bin(t->hour);
        t->day    = bcd_to_bin(t->day);
        t->month  = bcd_to_bin(t->month);
        t->year   = (uint8_t)bcd_to_bin((uint8_t)t->year);
    }

    /* Convert 24-hour mode: if bit 1 of Status B is NOT set, it's 12h */
    if (!(status_b & 0x02u) && (t->hour & 0x80u)) {
        /* 12h mode, PM: bit 7 is the PM marker */
        t->hour = (uint8_t)(((t->hour & 0x7Fu) + 12u) % 24u);
    }

    /* Y2K correction: 2-digit year from CMOS */
    t->year = (uint16_t)(t->year + (t->year < 70u ? 2000u : 1900u));
}

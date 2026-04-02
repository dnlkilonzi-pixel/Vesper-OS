#include "ata.h"
#include "port_io.h"

/* -------------------------------------------------------------------------
 * Primary ATA channel I/O ports
 * ---------------------------------------------------------------------- */
#define ATA_DATA        0x1F0u   /* 16-bit data register                 */
#define ATA_ERROR       0x1F1u   /* error / features                     */
#define ATA_SECT_COUNT  0x1F2u   /* sector count                         */
#define ATA_LBA_LOW     0x1F3u   /* LBA bits  0-7                        */
#define ATA_LBA_MID     0x1F4u   /* LBA bits  8-15                       */
#define ATA_LBA_HIGH    0x1F5u   /* LBA bits 16-23                       */
#define ATA_DRIVE_HEAD  0x1F6u   /* drive select + LBA bits 24-27        */
#define ATA_STATUS      0x1F7u   /* status (read)                        */
#define ATA_COMMAND     0x1F7u   /* command (write)                      */
#define ATA_ALT_STATUS  0x3F6u   /* alternate status (read, no IRQ ack)  */

/* Status register bits */
#define ATA_SR_BSY   0x80u   /* busy                        */
#define ATA_SR_DRDY  0x40u   /* drive ready                 */
#define ATA_SR_DRQ   0x08u   /* data request (ready to xfer) */
#define ATA_SR_ERR   0x01u   /* error                       */

/* Commands */
#define ATA_CMD_READ   0x20u   /* read sectors (PIO, with retry) */
#define ATA_CMD_WRITE  0x30u   /* write sectors (PIO, with retry) */
#define ATA_CMD_IDENT  0xECu   /* identify drive                  */

/* -------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------- */

/* 400 ns delay: read ALT_STATUS 4 times (each IN takes ~100 ns on real hw) */
static void ata_delay_400ns(void)
{
    inb(ATA_ALT_STATUS);
    inb(ATA_ALT_STATUS);
    inb(ATA_ALT_STATUS);
    inb(ATA_ALT_STATUS);
}

/* Wait until BSY is clear; return -1 on timeout */
static int ata_wait_not_busy(void)
{
    for (uint32_t i = 0; i < 0x100000u; i++) {
        if (!(inb(ATA_STATUS) & ATA_SR_BSY)) {
            return 0;
        }
    }
    return -1;
}

/* Wait until BSY=0 and DRQ=1 (ready to transfer data) */
static int ata_wait_drq(void)
{
    for (uint32_t i = 0; i < 0x100000u; i++) {
        uint8_t s = inb(ATA_STATUS);
        if (s & ATA_SR_ERR) {
            return -1;
        }
        if (!(s & ATA_SR_BSY) && (s & ATA_SR_DRQ)) {
            return 0;
        }
    }
    return -1;
}

/* -------------------------------------------------------------------------
 * ata_init – identify the primary master drive
 * ---------------------------------------------------------------------- */
int ata_init(void)
{
    /* Select drive 0, LBA mode */
    outb(ATA_DRIVE_HEAD, 0xE0u);
    ata_delay_400ns();

    /* Issue IDENTIFY */
    outb(ATA_SECT_COUNT, 0);
    outb(ATA_LBA_LOW,    0);
    outb(ATA_LBA_MID,    0);
    outb(ATA_LBA_HIGH,   0);
    outb(ATA_COMMAND,    ATA_CMD_IDENT);

    uint8_t status = inb(ATA_STATUS);
    if (status == 0x00u) {
        return 0;   /* no drive */
    }

    if (ata_wait_not_busy() != 0) {
        return 0;   /* timed out */
    }

    /* Check whether mid/high LBA ports were set (non-ATA device) */
    if (inb(ATA_LBA_MID) != 0 || inb(ATA_LBA_HIGH) != 0) {
        return 0;   /* ATAPI or other non-standard device */
    }

    if (ata_wait_drq() != 0) {
        return 0;
    }

    /* Read 256 words of identity data (discard) */
    for (int i = 0; i < 256; i++) {
        (void)inw(ATA_DATA);
    }

    return 1;   /* drive present */
}

/* -------------------------------------------------------------------------
 * ata_read_sectors – read @count sectors at LBA @lba into @buf
 * ---------------------------------------------------------------------- */
int ata_read_sectors(uint32_t lba, uint8_t count, void *buf)
{
    if (count == 0) {
        return 0;
    }

    if (ata_wait_not_busy() != 0) {
        return -1;
    }

    /* LBA 28-bit addressing: drive 0 = 0xE0, drive 1 = 0xF0 */
    outb(ATA_DRIVE_HEAD, (uint8_t)(0xE0u | ((lba >> 24u) & 0x0Fu)));
    outb(ATA_SECT_COUNT, count);
    outb(ATA_LBA_LOW,    (uint8_t)(lba & 0xFFu));
    outb(ATA_LBA_MID,    (uint8_t)((lba >> 8u)  & 0xFFu));
    outb(ATA_LBA_HIGH,   (uint8_t)((lba >> 16u) & 0xFFu));
    outb(ATA_COMMAND,    ATA_CMD_READ);

    uint16_t *p = (uint16_t *)buf;

    for (uint8_t s = 0; s < count; s++) {
        if (ata_wait_drq() != 0) {
            return -1;
        }
        /* Read 256 16-bit words per sector */
        for (int i = 0; i < 256; i++) {
            *p++ = inw(ATA_DATA);
        }
        ata_delay_400ns();
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * ata_write_sectors – write @count sectors from @buf to LBA @lba
 * ---------------------------------------------------------------------- */
int ata_write_sectors(uint32_t lba, uint8_t count, const void *buf)
{
    if (count == 0) {
        return 0;
    }

    if (ata_wait_not_busy() != 0) {
        return -1;
    }

    outb(ATA_DRIVE_HEAD, (uint8_t)(0xE0u | ((lba >> 24u) & 0x0Fu)));
    outb(ATA_SECT_COUNT, count);
    outb(ATA_LBA_LOW,    (uint8_t)(lba & 0xFFu));
    outb(ATA_LBA_MID,    (uint8_t)((lba >> 8u)  & 0xFFu));
    outb(ATA_LBA_HIGH,   (uint8_t)((lba >> 16u) & 0xFFu));
    outb(ATA_COMMAND,    ATA_CMD_WRITE);

    const uint16_t *p = (const uint16_t *)buf;

    for (uint8_t s = 0; s < count; s++) {
        if (ata_wait_drq() != 0) {
            return -1;
        }
        for (int i = 0; i < 256; i++) {
            outw(ATA_DATA, *p++);
        }
        /* Flush write cache */
        ata_delay_400ns();
    }

    return 0;
}

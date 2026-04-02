#ifndef ATA_H
#define ATA_H

#include <stdint.h>

/*
 * VESPER OS – ATA PIO driver (primary channel, drive 0)
 *
 * Communicates with the first IDE/ATA hard disk using 28-bit LBA PIO mode.
 * All I/O is polling (no DMA, no interrupts) for simplicity.
 *
 * QEMU presents the disk image (-drive format=raw,...) as the primary master
 * ATA device, accessible on I/O ports 0x1F0-0x1F7.
 */

/* Sector size in bytes (standard ATA) */
#define ATA_SECTOR_SIZE  512u

/*
 * ata_init – detect and identify drive 0 on the primary ATA bus.
 * Returns 1 if a drive is present, 0 otherwise.
 */
int ata_init(void);

/*
 * ata_read_sectors – read @count 512-byte sectors starting at LBA @lba
 *                    into the buffer at @buf.
 * Returns 0 on success, -1 on error.
 */
int ata_read_sectors(uint32_t lba, uint8_t count, void *buf);

/*
 * ata_write_sectors – write @count 512-byte sectors from @buf
 *                     to disk starting at LBA @lba.
 * Returns 0 on success, -1 on error.
 */
int ata_write_sectors(uint32_t lba, uint8_t count, const void *buf);

#endif /* ATA_H */

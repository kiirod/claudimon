#ifndef DISK_H
#define DISK_H

#include "stdint.h"

/* ============================================
   ATA PIO DISK DRIVER

   Talks to the primary IDE/ATA hard disk using
   old-school Programmed I/O (PIO) mode — simple,
   slow, but works everywhere including QEMU.

   We read/write in 512-byte "sectors" — the
   smallest unit a disk understands.
   ============================================ */

#define SECTOR_SIZE 512

/* Initialise the disk (checks it's present) */
int disk_init(void);

/* Read one 512-byte sector into buf */
int disk_read_sector(uint32_t lba, uint8_t* buf);

/* Write one 512-byte sector from buf */
int disk_write_sector(uint32_t lba, const uint8_t* buf);

#endif

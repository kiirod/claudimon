#include "disk.h"

/* ============================================
   ATA PIO DRIVER (Primary Bus, Master Drive)

   I/O Ports (primary ATA bus):
     0x1F0  Data port (read/write 16-bit words)
     0x1F1  Error / Features
     0x1F2  Sector count
     0x1F3  LBA low byte
     0x1F4  LBA mid byte
     0x1F5  LBA high byte
     0x1F6  Drive/head select
     0x1F7  Command / Status
   ============================================ */

#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECCOUNT    0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30

#define ATA_SR_BSY      0x80  /* Busy */
#define ATA_SR_DRQ      0x08  /* Data Request (ready to transfer) */
#define ATA_SR_ERR      0x01  /* Error */

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint16_t inw(uint16_t port) {
    uint16_t val;
    __asm__ volatile ("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* Wait until the disk is no longer busy */
static void ata_wait_busy(void) {
    while (inb(ATA_STATUS) & ATA_SR_BSY);
}

/* Wait until the disk has data ready to read/write */
static int ata_wait_drq(void) {
    uint8_t status;
    int timeout = 1000000;
    while (timeout--) {
        status = inb(ATA_STATUS);
        if (status & ATA_SR_ERR) return -1;
        if (status & ATA_SR_DRQ) return 0;
    }
    return -1; /* timeout */
}

int disk_init(void) {
    /* Select master drive */
    outb(ATA_DRIVE_HEAD, 0xE0);
    ata_wait_busy();

    uint8_t status = inb(ATA_STATUS);
    if (status == 0) return -1; /* no drive present */
    return 0;
}

int disk_read_sector(uint32_t lba, uint8_t* buf) {
    ata_wait_busy();

    /* Select drive + top 4 bits of LBA (LBA28 addressing) */
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_READ);

    if (ata_wait_drq() != 0) return -1;

    /* Read 256 16-bit words = 512 bytes */
    uint16_t* buf16 = (uint16_t*)buf;
    for (int i = 0; i < SECTOR_SIZE / 2; i++) {
        buf16[i] = inw(ATA_DATA);
    }
    return 0;
}

int disk_write_sector(uint32_t lba, const uint8_t* buf) {
    ata_wait_busy();

    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    if (ata_wait_drq() != 0) return -1;

    const uint16_t* buf16 = (const uint16_t*)buf;
    for (int i = 0; i < SECTOR_SIZE / 2; i++) {
        outw(ATA_DATA, buf16[i]);
    }

    /* Flush cache so data actually lands on "disk" */
    outb(ATA_COMMAND, 0xE7);
    ata_wait_busy();
    return 0;
}

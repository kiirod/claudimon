#include "power.h"
#include "stdint.h"

/* ============================================
   POWER MANAGEMENT

   Real ACPI shutdown needs parsing firmware
   tables (DSDT/FADT) — a substantial driver on
   its own. Instead we use the well-known "magic
   port" shortcuts that QEMU (and Bochs) expose
   for exactly this purpose; they're standard
   enough that almost every hobby kernel uses
   them for development shutdown support.
   ============================================ */

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void power_shutdown(void) {
    /* QEMU's "isa-debug-exit" / older QEMU ACPI shutdown port.
       Newer QEMU (5.0+) uses 0x604 with value 0x2000. */
    outw(0x604, 0x2000);

    /* Older QEMU versions used port 0xB004 */
    outw(0xB004, 0x2000);

    /* Bochs / very old QEMU shutdown port */
    outw(0x4004, 0x3400);

    /* If none of the above worked (e.g. real hardware), fall back to halt */
    power_halt();
}

void power_halt(void) {
    __asm__ volatile ("cli");   /* disable interrupts */
    while (1) {
        __asm__ volatile ("hlt");
    }
}

void power_reboot(void) {
    /* 8042 keyboard controller reset trick — pulses the CPU reset line.
       This is the classic, widely-compatible way to reboot from real mode
       and protected mode alike. */
    uint8_t good = 0x02;
    while (good & 0x02) {
        __asm__ volatile ("inb $0x64, %0" : "=a"(good));
    }
    outb(0x64, 0xFE);

    /* If the controller reset didn't work, just halt */
    power_halt();
}

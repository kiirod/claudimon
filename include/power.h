#ifndef POWER_H
#define POWER_H

/* Attempts to power off the virtual machine.
   Works in QEMU (and Bochs). On real hardware
   without ACPI support, this would just halt
   instead of actually powering off. */
void power_shutdown(void);

/* Halts the CPU without powering off (safe stop) */
void power_halt(void);

/* Performs a warm reboot back to GRUB */
void power_reboot(void);

#endif

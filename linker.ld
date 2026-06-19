/* linker.ld - Tells the linker how to arrange our kernel in memory
 *
 * When GRUB loads our kernel, it places it at 1MB (0x100000) in memory.
 * This script tells the linker exactly where each section goes.
 */

ENTRY(_start)   /* The very first instruction is _start in boot.asm */

SECTIONS {
    /*
     * Start at 1MB (0x100000).
     * The first megabyte of memory is used by BIOS and hardware.
     * It's traditional (and safe) to load kernels at 1MB.
     */
    . = 1M;

    /* 
     * .multiboot_header must come FIRST so GRUB can find
     * the magic signature quickly at the start of the file.
     */
    .multiboot_header : {
        *(.multiboot_header)
    }

    /* 
     * .text = executable code
     * All our compiled functions end up here.
     */
    .text : {
        *(.text)
    }

    /*
     * .rodata = read-only data
     * String literals (like "Hello, Kernel!") go here.
     */
    .rodata : {
        *(.rodata)
    }

    /*
     * .data = initialised global/static variables
     */
    .data : {
        *(.data)
    }

    /*
     * .bss = uninitialised global/static variables (our stack lives here)
     * The loader zeroes this section automatically.
     */
    .bss : {
        *(COMMON)
        *(.bss)
    }
}

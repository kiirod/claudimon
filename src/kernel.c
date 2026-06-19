#include "vga.h"

/* ============================================
   KERNEL MAIN
   
   This is where our kernel "starts" from C's
   perspective. boot.asm sets up the stack and
   then calls this function.
   ============================================ */

void kernel_main(void) {

    /* Step 1: Initialise the terminal (clear screen) */
    terminal_init();

    /* Step 2: Print a welcome banner */
    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_write("======================================\n");
    terminal_write("        Welcome to Claudimon!          \n");
    terminal_write("======================================\n");
    terminal_write("\n");

    /* Step 3: Hello world! */
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    terminal_write("Hello, Kernel!\n");
    terminal_write("\n");

    /* Step 4: Show off some colours */
    terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_write("[OK] VGA text mode driver loaded\n");

    terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_write("[OK] Kernel started successfully\n");

    terminal_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    terminal_write("[--] Waiting for more features to be built...\n");

    terminal_write("\n");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write("This is your kernel. Build something great!\n");

    /* The kernel must never return - just hang here */
    while (1) {
        /* In a real kernel you'd handle interrupts,
           schedule processes, etc. here */
        __asm__("hlt");  /* Sleep until an interrupt wakes us */
    }
}

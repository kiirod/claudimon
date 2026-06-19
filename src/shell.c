#include "shell.h"
#include "vga.h"
#include "keyboard.h"

/* ============================================
   SHELL
   
   A basic interactive shell that:
   - Displays a prompt
   - Reads a line of input from the keyboard
   - Parses and executes commands
   ============================================ */

#define CMD_BUFFER_SIZE 256

/* ---- Tiny string helpers (no libc!) ---- */

static int k_strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static int k_strncmp(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
        if (a[i] == '\0') return 0;
    }
    return 0;
}

static int k_strlen(const char* s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

/* ============================================
   COMMANDS
   ============================================ */

static void cmd_help(void) {
    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_write("\nAvailable commands:\n");
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    terminal_write("  help          - Show this help message\n");
    terminal_write("  clear         - Clear the screen\n");
    terminal_write("  echo <text>   - Print text to the screen\n");
    terminal_write("  info          - Show system information\n");
    terminal_write("  mem           - Show memory information\n");
    terminal_write("  cpu           - Show CPU information\n");
    terminal_write("  color         - Demo all terminal colours\n");
    terminal_write("  version       - Show kernel version\n");
    terminal_write("\n");
}

static void cmd_clear(void) {
    terminal_init();
}

static void cmd_echo(const char* args) {
    if (args && *args) {
        terminal_write(args);
        terminal_write("\n");
    } else {
        terminal_write("\n");
    }
}

static void cmd_version(void) {
    terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_write("\nMyKernel v0.1.0\n");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write("Built with love in C + Assembly\n\n");
}

static void cmd_info(void) {
    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_write("\n=== System Information ===\n");
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    terminal_write("  Kernel   : MyKernel v0.1.0\n");
    terminal_write("  Arch     : x86 (32-bit protected mode)\n");
    terminal_write("  Bootloader: GRUB (Multiboot2)\n");
    terminal_write("  Display  : VGA Text Mode 80x25\n");
    terminal_write("  Input    : PS/2 Keyboard via IRQ1\n");
    terminal_write("\n");
}

static void cmd_mem(void) {
    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_write("\n=== Memory Information ===\n");
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    terminal_write("  Kernel load address : 0x00100000 (1 MB)\n");
    terminal_write("  VGA text buffer     : 0x000B8000\n");
    terminal_write("  Stack size          : 16 KB\n");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write("  (Full memory map requires a memory manager)\n\n");
}

static void cmd_cpu(void) {
    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_write("\n=== CPU Information ===\n");
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    /* Use CPUID instruction to get CPU vendor string */
    uint32_t ebx, ecx, edx;
    __asm__ volatile (
        "cpuid"
        : "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );

    /* Vendor string is 12 bytes split across EBX, EDX, ECX */
    char vendor[13];
    vendor[0]  = (ebx      ) & 0xFF;
    vendor[1]  = (ebx >>  8) & 0xFF;
    vendor[2]  = (ebx >> 16) & 0xFF;
    vendor[3]  = (ebx >> 24) & 0xFF;
    vendor[4]  = (edx      ) & 0xFF;
    vendor[5]  = (edx >>  8) & 0xFF;
    vendor[6]  = (edx >> 16) & 0xFF;
    vendor[7]  = (edx >> 24) & 0xFF;
    vendor[8]  = (ecx      ) & 0xFF;
    vendor[9]  = (ecx >>  8) & 0xFF;
    vendor[10] = (ecx >> 16) & 0xFF;
    vendor[11] = (ecx >> 24) & 0xFF;
    vendor[12] = '\0';

    terminal_write("  Vendor  : ");
    terminal_write(vendor);
    terminal_write("\n");
    terminal_write("  Mode    : 32-bit Protected Mode\n");
    terminal_write("  CPUID   : Supported\n\n");
}

static void cmd_color(void) {
    terminal_write("\nColour palette:\n");
    vga_color_t colors[] = {
        VGA_COLOR_BLACK, VGA_COLOR_BLUE, VGA_COLOR_GREEN, VGA_COLOR_CYAN,
        VGA_COLOR_RED, VGA_COLOR_MAGENTA, VGA_COLOR_BROWN, VGA_COLOR_LIGHT_GREY,
        VGA_COLOR_DARK_GREY, VGA_COLOR_LIGHT_BLUE, VGA_COLOR_LIGHT_GREEN,
        VGA_COLOR_LIGHT_CYAN, VGA_COLOR_LIGHT_RED, VGA_COLOR_LIGHT_MAGENTA,
        VGA_COLOR_LIGHT_BROWN, VGA_COLOR_WHITE
    };
    const char* names[] = {
        "BLACK      ","BLUE       ","GREEN      ","CYAN       ",
        "RED        ","MAGENTA    ","BROWN      ","LIGHT_GREY ",
        "DARK_GREY  ","LIGHT_BLUE ","LIGHT_GREEN",
        "LIGHT_CYAN ","LIGHT_RED  ","LIGHT_MAG  ",
        "YELLOW     ","WHITE      "
    };
    for (int i = 0; i < 16; i++) {
        terminal_set_color(colors[i], VGA_COLOR_BLACK);
        terminal_write("  ");
        terminal_write(names[i]);
        if (i % 2 == 1) terminal_write("\n");
    }
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    terminal_write("\n");
}

/* ============================================
   COMMAND PARSER
   ============================================ */

static void execute(char* cmd) {
    /* Skip leading spaces */
    while (*cmd == ' ') cmd++;

    /* Empty line — do nothing */
    if (*cmd == '\0') return;

    if (k_strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (k_strcmp(cmd, "clear") == 0) {
        cmd_clear();
    } else if (k_strcmp(cmd, "version") == 0) {
        cmd_version();
    } else if (k_strcmp(cmd, "info") == 0) {
        cmd_info();
    } else if (k_strcmp(cmd, "mem") == 0) {
        cmd_mem();
    } else if (k_strcmp(cmd, "cpu") == 0) {
        cmd_cpu();
    } else if (k_strcmp(cmd, "color") == 0) {
        cmd_color();
    } else if (k_strncmp(cmd, "echo ", 5) == 0) {
        cmd_echo(cmd + 5);
    } else if (k_strcmp(cmd, "echo") == 0) {
        cmd_echo("");
    } else {
        terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        terminal_write("Unknown command: '");
        terminal_write(cmd);
        terminal_write("'  (type 'help' for a list)\n");
        terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
}

/* ============================================
   MAIN SHELL LOOP
   ============================================ */

static void print_prompt(void) {
    terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_write("mykernel");
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    terminal_write("> ");
}

void shell_run(void) {
    char buf[CMD_BUFFER_SIZE];
    int  pos = 0;

    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_write("======================================\n");
    terminal_write("        Welcome to MyKernel!          \n");
    terminal_write("======================================\n");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write("Type 'help' to see available commands.\n\n");
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    while (1) {
        print_prompt();
        pos = 0;

        /* Read a line of input */
        while (1) {
            char c = keyboard_getchar();

            if (c == '\n') {
                /* Enter pressed — execute the command */
                terminal_putchar('\n');
                buf[pos] = '\0';
                execute(buf);
                break;

            } else if (c == '\b' || c == 127) {
                /* Backspace — delete last character */
                if (pos > 0) {
                    pos--;
                    /* Move cursor back, print space, move back again */
                    terminal_putchar('\b');
                    terminal_putchar(' ');
                    terminal_putchar('\b');
                }

            } else if (pos < CMD_BUFFER_SIZE - 1) {
                /* Normal character — add to buffer and echo */
                buf[pos++] = c;
                terminal_putchar(c);
            }
        }
    }
}

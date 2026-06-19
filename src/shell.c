#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "fs.h"
#include "speaker.h"
#include "theme.h"

#define CMD_BUFFER_SIZE 256

/* ---- String helpers ---- */
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

/* ============================================
   COMMANDS
   ============================================ */

static void cmd_help(void) {
    terminal_set_color(theme_banner_color, VGA_COLOR_BLACK);
    terminal_write("\nClaudimon Commands\n");
    terminal_set_color(theme_text_color, VGA_COLOR_BLACK);
    terminal_write("------------------\n");
    terminal_write("  help            - This help message\n");
    terminal_write("  clear           - Clear the screen\n");
    terminal_write("  echo <text>     - Print text\n");
    terminal_write("  version         - Kernel version\n");
    terminal_write("  info            - System information\n");
    terminal_write("  mem             - Memory information\n");
    terminal_write("  cpu             - CPU information\n");
    terminal_write("  color           - Show colour palette\n");
    terminal_write("  theme           - List colour themes\n");
    terminal_write("  theme <n>       - Apply theme by number\n");
    terminal_write("  ls              - List current directory\n");
    terminal_write("  mkdir <name>    - Create a directory\n");
    terminal_write("  cd <name>       - Change directory\n");
    terminal_write("  pwd             - Print working directory\n");
    terminal_write("  cat <file>      - Print file contents\n");
    terminal_write("  touch <f> <txt> - Create a file with text\n");
    terminal_write("  beep            - Play a beep\n");
    terminal_write("  play <freq>     - Play a tone (Hz)\n");
    terminal_write("  jingle          - Play the boot jingle\n");
    terminal_write("\n");
}

static void cmd_clear(void) { terminal_init(); }

static void cmd_echo(const char* args) {
    terminal_write(args && *args ? args : "");
    terminal_write("\n");
}

static void cmd_version(void) {
    terminal_set_color(theme_success_color, VGA_COLOR_BLACK);
    terminal_write("\nClaudimon v0.2.0\n");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write("Built in C + Assembly\n\n");
}

static void cmd_info(void) {
    terminal_set_color(theme_banner_color, VGA_COLOR_BLACK);
    terminal_write("\n=== System Information ===\n");
    terminal_set_color(theme_text_color, VGA_COLOR_BLACK);
    terminal_write("  Kernel     : Claudimon v0.2.0\n");
    terminal_write("  Arch       : x86 (32-bit protected mode)\n");
    terminal_write("  Bootloader : GRUB (Multiboot2)\n");
    terminal_write("  Display    : VGA Text Mode 80x25\n");
    terminal_write("  Input      : PS/2 Keyboard (polling)\n");
    terminal_write("  Filesystem : In-memory\n");
    terminal_write("  Audio      : PC Speaker (PIT channel 2)\n\n");
}

static void cmd_mem(void) {
    terminal_set_color(theme_banner_color, VGA_COLOR_BLACK);
    terminal_write("\n=== Memory ===\n");
    terminal_set_color(theme_text_color, VGA_COLOR_BLACK);
    terminal_write("  Kernel load : 0x00100000 (1 MB)\n");
    terminal_write("  VGA buffer  : 0x000B8000\n");
    terminal_write("  Stack       : 16 KB\n\n");
}

static void cmd_cpu(void) {
    terminal_set_color(theme_banner_color, VGA_COLOR_BLACK);
    terminal_write("\n=== CPU ===\n");
    terminal_set_color(theme_text_color, VGA_COLOR_BLACK);
    uint32_t ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(0));
    char vendor[13];
    vendor[0]=(ebx)&0xFF; vendor[1]=(ebx>>8)&0xFF;
    vendor[2]=(ebx>>16)&0xFF; vendor[3]=(ebx>>24)&0xFF;
    vendor[4]=(edx)&0xFF; vendor[5]=(edx>>8)&0xFF;
    vendor[6]=(edx>>16)&0xFF; vendor[7]=(edx>>24)&0xFF;
    vendor[8]=(ecx)&0xFF; vendor[9]=(ecx>>8)&0xFF;
    vendor[10]=(ecx>>16)&0xFF; vendor[11]=(ecx>>24)&0xFF;
    vendor[12]='\0';
    terminal_write("  Vendor : "); terminal_write(vendor); terminal_write("\n");
    terminal_write("  Mode   : 32-bit Protected Mode\n\n");
}

static void cmd_color(void) {
    terminal_write("\nColour palette:\n");
    vga_color_t colors[] = {
        VGA_COLOR_BLACK,VGA_COLOR_BLUE,VGA_COLOR_GREEN,VGA_COLOR_CYAN,
        VGA_COLOR_RED,VGA_COLOR_MAGENTA,VGA_COLOR_BROWN,VGA_COLOR_LIGHT_GREY,
        VGA_COLOR_DARK_GREY,VGA_COLOR_LIGHT_BLUE,VGA_COLOR_LIGHT_GREEN,
        VGA_COLOR_LIGHT_CYAN,VGA_COLOR_LIGHT_RED,VGA_COLOR_LIGHT_MAGENTA,
        VGA_COLOR_LIGHT_BROWN,VGA_COLOR_WHITE
    };
    const char* names[] = {
        "BLACK    ","BLUE     ","GREEN    ","CYAN     ",
        "RED      ","MAGENTA  ","BROWN    ","LT_GREY  ",
        "DK_GREY  ","LT_BLUE  ","LT_GREEN ",
        "LT_CYAN  ","LT_RED   ","LT_MAG   ",
        "YELLOW   ","WHITE    "
    };
    for (int i = 0; i < 16; i++) {
        terminal_set_color(colors[i], VGA_COLOR_BLACK);
        terminal_write("  "); terminal_write(names[i]);
        if (i % 2 == 1) terminal_write("\n");
    }
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    terminal_write("\n");
}

/* Parse a simple number from string */
static uint32_t parse_uint(const char* s) {
    uint32_t n = 0;
    while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; }
    return n;
}

static void cmd_play(const char* args) {
    if (!args || !*args) {
        terminal_write("Usage: play <frequency>\n");
        return;
    }
    uint32_t freq = parse_uint(args);
    if (freq < 20 || freq > 20000) {
        terminal_set_color(theme_error_color, VGA_COLOR_BLACK);
        terminal_write("Frequency must be between 20 and 20000 Hz\n");
        return;
    }
    speaker_play(freq);
    /* play for ~500ms then stop */
    for (volatile int i = 0; i < 2000000; i++) __asm__("nop");
    speaker_stop();
}

/* touch <filename> <text...> */
static void cmd_touch(const char* args) {
    if (!args || !*args) { terminal_write("Usage: touch <name> <text>\n"); return; }
    char name[FS_NAME_LEN];
    int i = 0;
    while (args[i] && args[i] != ' ' && i < FS_NAME_LEN - 1) {
        name[i] = args[i]; i++;
    }
    name[i] = '\0';
    const char* data = (args[i] == ' ') ? args + i + 1 : "";
    if (fs_create(name, data) >= 0) {
        terminal_set_color(theme_success_color, VGA_COLOR_BLACK);
        terminal_write("Created: "); terminal_write(name); terminal_write("\n");
    }
}

static void cmd_theme(const char* args) {
    if (!args || !*args) { theme_list(); return; }
    int n = args[0] - '0';
    theme_apply(n);
}

/* ============================================
   COMMAND PARSER
   ============================================ */

static void execute(char* cmd) {
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') return;

    if      (k_strcmp(cmd, "help")    == 0) cmd_help();
    else if (k_strcmp(cmd, "clear")   == 0) cmd_clear();
    else if (k_strcmp(cmd, "version") == 0) cmd_version();
    else if (k_strcmp(cmd, "info")    == 0) cmd_info();
    else if (k_strcmp(cmd, "mem")     == 0) cmd_mem();
    else if (k_strcmp(cmd, "cpu")     == 0) cmd_cpu();
    else if (k_strcmp(cmd, "color")   == 0) cmd_color();
    else if (k_strcmp(cmd, "ls")      == 0) fs_ls();
    else if (k_strcmp(cmd, "pwd")     == 0) fs_pwd();
    else if (k_strcmp(cmd, "beep")    == 0) speaker_beep();
    else if (k_strcmp(cmd, "jingle")  == 0) speaker_boot_sound();
    else if (k_strncmp(cmd, "echo ",   5) == 0) cmd_echo(cmd + 5);
    else if (k_strncmp(cmd, "mkdir ",  6) == 0) fs_mkdir(cmd + 6);
    else if (k_strncmp(cmd, "cd ",     3) == 0) fs_cd(cmd + 3);
    else if (k_strncmp(cmd, "cat ",    4) == 0) fs_cat(cmd + 4);
    else if (k_strncmp(cmd, "touch ",  6) == 0) cmd_touch(cmd + 6);
    else if (k_strncmp(cmd, "play ",   5) == 0) cmd_play(cmd + 5);
    else if (k_strncmp(cmd, "theme ",  6) == 0) cmd_theme(cmd + 6);
    else if (k_strcmp(cmd, "theme")   == 0) theme_list();
    else if (k_strcmp(cmd, "echo")    == 0) cmd_echo("");
    else {
        terminal_set_color(theme_error_color, VGA_COLOR_BLACK);
        terminal_write("Unknown command: '");
        terminal_write(cmd);
        terminal_write("'  (type 'help')\n");
        speaker_error_sound();
    }
    terminal_set_color(theme_text_color, VGA_COLOR_BLACK);
}

/* ============================================
   SHELL LOOP
   ============================================ */

static void print_prompt(void) {
    terminal_set_color(theme_prompt_color, VGA_COLOR_BLACK);
    terminal_write("claudimon");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write("> ");
    terminal_set_color(theme_text_color, VGA_COLOR_BLACK);
}

void shell_run(void) {
    char buf[CMD_BUFFER_SIZE];
    int  pos = 0;

    /* Apply default theme */
    theme_apply(0);

    terminal_set_color(theme_banner_color, VGA_COLOR_BLACK);
    terminal_write("======================================\n");
    terminal_write("         Welcome to Claudimon!        \n");
    terminal_write("======================================\n");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write("Type 'help' to see available commands.\n\n");

    speaker_boot_sound();

    while (1) {
        print_prompt();
        pos = 0;

        while (1) {
            char c = keyboard_getchar();

            if (c == '\n') {
                terminal_putchar('\n');
                buf[pos] = '\0';
                execute(buf);
                break;
            } else if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    terminal_putchar('\b');
                    terminal_putchar(' ');
                    terminal_putchar('\b');
                }
            } else if (pos < CMD_BUFFER_SIZE - 1) {
                buf[pos++] = c;
                terminal_putchar(c);
            }
        }
    }
}

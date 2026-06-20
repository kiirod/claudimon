#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "fs.h"
#include "speaker.h"
#include "theme.h"
#include "editor.h"
#include "power.h"
#include "stdint.h"

extern vga_color_t theme_banner_color;
extern vga_color_t theme_prompt_color;
extern vga_color_t theme_text_color;
extern vga_color_t theme_success_color;
extern vga_color_t theme_error_color;
extern vga_color_t theme_dir_color;
extern uint32_t total_memory_kb;

#define CMD_BUFFER_SIZE 256
#define HISTORY_SIZE    10

/* ---- History ---- */
static char history[HISTORY_SIZE][CMD_BUFFER_SIZE];
static int  history_count = 0;
static int  history_pos   = 0;  /* position when scrolling */

static void history_add(const char* cmd) {
    if (cmd[0] == '\0') return;
    /* Don't duplicate last entry */
    if (history_count > 0) {
        int last = (history_count - 1) % HISTORY_SIZE;
        int same = 1;
        for (int i = 0; cmd[i] || history[last][i]; i++) {
            if (cmd[i] != history[last][i]) { same = 0; break; }
        }
        if (same) return;
    }
    int slot = history_count % HISTORY_SIZE;
    int i = 0;
    while (cmd[i] && i < CMD_BUFFER_SIZE - 1) { history[slot][i] = cmd[i]; i++; }
    history[slot][i] = '\0';
    history_count++;
}

static const char* history_get(int offset) {
    /* offset 1 = last cmd, 2 = second last, etc. */
    if (offset <= 0 || offset > history_count || offset > HISTORY_SIZE) return 0;
    int idx = ((history_count - offset) % HISTORY_SIZE + HISTORY_SIZE) % HISTORY_SIZE;
    return history[idx];
}

/* ---- Path tracking ---- */
#define MAX_DEPTH 16
#define MAX_SEG   32
static char path_segments[MAX_DEPTH][MAX_SEG];
static int  path_depth = 0;

static void path_push(const char* name) {
    if (path_depth < MAX_DEPTH) {
        int i = 0;
        while (name[i] && i < MAX_SEG-1) { path_segments[path_depth][i]=name[i]; i++; }
        path_segments[path_depth][i] = '\0';
        path_depth++;
    }
}
static void path_pop(void)   { if (path_depth > 0) path_depth--; }
static void path_reset(void) { path_depth = 0; }

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
static void uint_to_str(uint32_t n, char* buf) {
    if (n == 0) { buf[0]='0'; buf[1]='\0'; return; }
    char tmp[16]; int i=0;
    while (n) { tmp[i++]='0'+(n%10); n/=10; }
    int j=0; while(i>0) buf[j++]=tmp[--i]; buf[j]='\0';
}

/* ============================================
   ASCII ART LOGO  (robot face from the icon)
   ============================================ */
static void print_logo(void) {
    terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    terminal_write("                                        \n");
    terminal_write("         .~################~.           \n");
    terminal_write("       :######################:         \n");
    terminal_write("      #####    .---.    #######         \n");
    terminal_write("     #####    ( o o )    ######         \n");
    terminal_write("     #####     `---'     ######         \n");
    terminal_write("      ######################            \n");
    terminal_write("       .##################.            \n");
    terminal_write("     :########################:         \n");
    terminal_write("      '######################'         \n");
    terminal_write("                                        \n");
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    terminal_write("            C L A U D I M O N          \n");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write("          Your kernel. Your rules.      \n");
    terminal_write("\n");
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
    terminal_write("  logo            - Show the boot logo\n");
    terminal_write("  echo <text>     - Print text\n");
    terminal_write("  version         - Kernel version\n");
    terminal_write("  info            - System information\n");
    terminal_write("  mem             - Memory information\n");
    terminal_write("  cpu             - CPU information\n");
    terminal_write("  color           - Show colour palette\n");
    terminal_write("  theme           - List colour themes\n");
    terminal_write("  theme <n>       - Apply theme (0-4)\n");
    terminal_write("  ls              - List directory\n");
    terminal_write("  mkdir <name>    - Create directory\n");
    terminal_write("  cd <name>       - Change directory\n");
    terminal_write("  cd ..           - Go up\n");
    terminal_write("  pwd             - Print path\n");
    terminal_write("  cat <file>      - Read file\n");
    terminal_write("  touch <f> <txt> - Create file\n");
    terminal_write("  edit <file>     - Open text editor\n");
    terminal_write("  history         - Show command history\n");
    terminal_write("  keyboard        - List keyboard layouts\n");
    terminal_write("  keyboard <n>    - Switch keyboard layout\n");
    terminal_write("  shutdown        - Power off Claudimon\n");
    terminal_write("  close           - Same as shutdown\n");
    terminal_write("  reboot          - Restart Claudimon\n");
    terminal_write("\nTip: Shift+Up / Shift+Down scrolls the screen history\n");
    terminal_write("     Up / Down (no shift) cycles command history\n");
    terminal_write("  calc <expr>     - e.g. calc 10 + 5\n");
    terminal_write("  beep            - Beep\n");
    terminal_write("  play <freq>     - Play tone (Hz)\n");
    terminal_write("  jingle          - Boot jingle\n\n");
}

static void cmd_clear(void) { terminal_init(); }

static void cmd_echo(const char* a) { terminal_write(a&&*a?a:""); terminal_write("\n"); }

static void cmd_version(void) {
    terminal_set_color(theme_success_color, VGA_COLOR_BLACK);
    terminal_write("\nClaudimon v0.4.0\n");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write("Built in C + Assembly\n\n");
}

static void cmd_info(void) {
    terminal_set_color(theme_banner_color, VGA_COLOR_BLACK);
    terminal_write("\n=== System Information ===\n");
    terminal_set_color(theme_text_color, VGA_COLOR_BLACK);
    terminal_write("  Kernel     : Claudimon v0.4.0\n");
    terminal_write("  Arch       : x86 32-bit protected mode\n");
    terminal_write("  Bootloader : GRUB (Multiboot2)\n");
    terminal_write("  Display    : VGA Text Mode 80x25\n");
    terminal_write("  Input      : PS/2 Keyboard (polling)\n");
    terminal_write("  Filesystem : In-memory\n");
    terminal_write("  Audio      : PC Speaker\n\n");
}

static void cmd_mem(void) {
    terminal_set_color(theme_banner_color, VGA_COLOR_BLACK);
    terminal_write("\n=== Memory ===\n");
    terminal_set_color(theme_text_color, VGA_COLOR_BLACK);
    char buf[16];
    uint_to_str(total_memory_kb, buf);
    terminal_write("  Total RAM   : "); terminal_write(buf); terminal_write(" KB (");
    uint_to_str(total_memory_kb/1024, buf);
    terminal_write(buf); terminal_write(" MB)\n");
    terminal_write("  Kernel load : 0x00100000 (1 MB)\n");
    terminal_write("  VGA buffer  : 0x000B8000\n");
    terminal_write("  Stack size  : 16 KB\n\n");
}

static void cmd_cpu(void) {
    terminal_set_color(theme_banner_color, VGA_COLOR_BLACK);
    terminal_write("\n=== CPU ===\n");
    terminal_set_color(theme_text_color, VGA_COLOR_BLACK);
    uint32_t ebx,ecx,edx;
    __asm__ volatile("cpuid":"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(0));
    char v[13];
    v[0]=(ebx)&0xFF;    v[1]=(ebx>>8)&0xFF;  v[2]=(ebx>>16)&0xFF; v[3]=(ebx>>24)&0xFF;
    v[4]=(edx)&0xFF;    v[5]=(edx>>8)&0xFF;  v[6]=(edx>>16)&0xFF; v[7]=(edx>>24)&0xFF;
    v[8]=(ecx)&0xFF;    v[9]=(ecx>>8)&0xFF;  v[10]=(ecx>>16)&0xFF;v[11]=(ecx>>24)&0xFF;
    v[12]='\0';
    terminal_write("  Vendor : "); terminal_write(v); terminal_write("\n");
    terminal_write("  Mode   : 32-bit Protected Mode\n\n");
}

static void cmd_color(void) {
    terminal_write("\nColour palette:\n");
    vga_color_t cols[]={VGA_COLOR_BLACK,VGA_COLOR_BLUE,VGA_COLOR_GREEN,VGA_COLOR_CYAN,
        VGA_COLOR_RED,VGA_COLOR_MAGENTA,VGA_COLOR_BROWN,VGA_COLOR_LIGHT_GREY,
        VGA_COLOR_DARK_GREY,VGA_COLOR_LIGHT_BLUE,VGA_COLOR_LIGHT_GREEN,VGA_COLOR_LIGHT_CYAN,
        VGA_COLOR_LIGHT_RED,VGA_COLOR_LIGHT_MAGENTA,VGA_COLOR_LIGHT_BROWN,VGA_COLOR_WHITE};
    const char* names[]={"BLACK    ","BLUE     ","GREEN    ","CYAN     ",
        "RED      ","MAGENTA  ","BROWN    ","LT_GREY  ",
        "DK_GREY  ","LT_BLUE  ","LT_GREEN ","LT_CYAN  ",
        "LT_RED   ","LT_MAG   ","YELLOW   ","WHITE    "};
    for(int i=0;i<16;i++){
        terminal_set_color(cols[i],VGA_COLOR_BLACK);
        terminal_write("  "); terminal_write(names[i]);
        if(i%2==1) terminal_write("\n");
    }
    terminal_set_color(VGA_COLOR_WHITE,VGA_COLOR_BLACK);
    terminal_write("\n");
}

static void cmd_history(void) {
    terminal_set_color(theme_banner_color, VGA_COLOR_BLACK);
    terminal_write("\nCommand History:\n");
    terminal_set_color(theme_text_color, VGA_COLOR_BLACK);
    int count = history_count < HISTORY_SIZE ? history_count : HISTORY_SIZE;
    if (count == 0) { terminal_write("  (empty)\n\n"); return; }
    for (int i = count; i >= 1; i--) {
        const char* h = history_get(i);
        if (h) {
            char num[4];
            uint_to_str(history_count - i + 1, num);
            terminal_write("  "); terminal_write(num);
            terminal_write("  "); terminal_write(h); terminal_write("\n");
        }
    }
    terminal_write("\n");
}

/* ---- Simple calculator ---- */
static void cmd_calc(const char* args) {
    if (!args || !*args) { terminal_write("Usage: calc <n> <op> <n>\nOps: + - * /\n"); return; }

    /* Parse: number op number */
    int32_t a = 0, b = 0;
    char op = 0;
    int neg = 0;
    const char* p = args;

    /* Skip spaces */
    while (*p == ' ') p++;
    if (*p == '-') { neg=1; p++; }
    while (*p >= '0' && *p <= '9') { a = a*10 + (*p-'0'); p++; }
    if (neg) a = -a;

    while (*p == ' ') p++;
    op = *p++;
    while (*p == ' ') p++;
    neg = 0;
    if (*p == '-') { neg=1; p++; }
    while (*p >= '0' && *p <= '9') { b = b*10 + (*p-'0'); p++; }
    if (neg) b = -b;

    int32_t result = 0;
    int valid = 1;
    if      (op == '+') result = a + b;
    else if (op == '-') result = a - b;
    else if (op == '*') result = a * b;
    else if (op == '/') {
        if (b == 0) { terminal_set_color(theme_error_color,VGA_COLOR_BLACK);
                      terminal_write("Error: divide by zero\n"); return; }
        result = a / b;
    } else { valid = 0; }

    if (!valid) { terminal_set_color(theme_error_color,VGA_COLOR_BLACK);
                  terminal_write("Unknown operator. Use + - * /\n"); return; }

    terminal_set_color(theme_success_color, VGA_COLOR_BLACK);
    terminal_write("= ");

    /* Print result (handle negative) */
    char buf[16]; int idx=0;
    int32_t r = result;
    if (r < 0) { terminal_write("-"); r = -r; }
    if (r == 0) { buf[idx++]='0'; }
    else { char tmp[12]; int ti=0; while(r){tmp[ti++]='0'+(r%10);r/=10;}
           while(ti>0) buf[idx++]=tmp[--ti]; }
    buf[idx]='\0';
    terminal_write(buf);
    terminal_write("\n");
}

static uint32_t parse_uint(const char* s) {
    uint32_t n=0;
    while(*s>='0'&&*s<='9'){n=n*10+(*s-'0');s++;}
    return n;
}

static void cmd_play(const char* args) {
    if(!args||!*args){terminal_write("Usage: play <freq>\n");return;}
    uint32_t freq=parse_uint(args);
    if(freq<20||freq>20000){
        terminal_set_color(theme_error_color,VGA_COLOR_BLACK);
        terminal_write("Frequency must be 20-20000 Hz\n"); return;
    }
    speaker_play(freq);
    for(volatile uint32_t i=0;i<8000000;i++) __asm__("nop");
    speaker_stop();
    terminal_set_color(theme_success_color,VGA_COLOR_BLACK);
    terminal_write("Done.\n");
}

static void cmd_touch(const char* args) {
    if(!args||!*args){terminal_write("Usage: touch <name> <text>\n");return;}
    char name[FS_NAME_LEN]; int i=0;
    while(args[i]&&args[i]!=' '&&i<FS_NAME_LEN-1){name[i]=args[i];i++;}
    name[i]='\0';
    const char* data=(args[i]==' ')?args+i+1:"";
    if(fs_create(name,data)>=0){
        terminal_set_color(theme_success_color,VGA_COLOR_BLACK);
        terminal_write("Created: ");terminal_write(name);terminal_write("\n");
    }
}

static void cmd_theme(const char* args) {
    if(!args||!*args){theme_list();return;}
    theme_apply(args[0]-'0');
}

static void cmd_cd(const char* args) {
    if(!args||!*args) return;
    if(k_strcmp(args,"..")==0){ if(fs_cd("..")==0) path_pop(); }
    else if(k_strcmp(args,"/")==0){ if(fs_cd("/")==0) path_reset(); }
    else { if(fs_cd(args)==0) path_push(args); }
}

static void cmd_keyboard(const char* args) {
    if (!args || !*args) {
        terminal_set_color(theme_banner_color, VGA_COLOR_BLACK);
        terminal_write("\nAvailable keyboard layouts:\n");
        terminal_set_color(theme_text_color, VGA_COLOR_BLACK);
        for (int i = 0; i < LAYOUT_COUNT; i++) {
            char num[4]; uint_to_str(i, num);
            if (i == (int)keyboard_get_layout()) terminal_write("  * ");
            else                                  terminal_write("    ");
            terminal_write(num);
            terminal_write(" - ");
            terminal_write(keyboard_layout_name((keyboard_layout_t)i));
            terminal_write("\n");
        }
        terminal_write("\nUsage: keyboard <number>\n\n");
        return;
    }
    int n = args[0] - '0';
    if (n < 0 || n >= LAYOUT_COUNT) {
        terminal_set_color(theme_error_color, VGA_COLOR_BLACK);
        terminal_write("Invalid layout number. Type 'keyboard' to see options.\n");
        return;
    }
    keyboard_set_layout((keyboard_layout_t)n);
    terminal_set_color(theme_success_color, VGA_COLOR_BLACK);
    terminal_write("Keyboard layout set to: ");
    terminal_write(keyboard_layout_name((keyboard_layout_t)n));
    terminal_write("\n");
}

static void cmd_edit(const char* args) {
    if(!args||!*args){terminal_write("Usage: edit <filename>\n");return;}
    editor_open(args);
}

static void cmd_shutdown(void) {
    terminal_set_color(theme_banner_color, VGA_COLOR_BLACK);
    terminal_write("\nShutting down Claudimon...\n");
    terminal_set_color(theme_text_color, VGA_COLOR_BLACK);
    terminal_write("Goodbye!\n");
    speaker_play(440);
    for (volatile uint32_t i = 0; i < 3000000; i++) __asm__("nop");
    speaker_stop();
    power_shutdown();
    /* If we ever reach here, shutdown wasn't supported by the host */
    terminal_set_color(theme_error_color, VGA_COLOR_BLACK);
    terminal_write("\nShutdown not supported on this machine — halting instead.\n");
    power_halt();
}

static void cmd_reboot(void) {
    terminal_set_color(theme_banner_color, VGA_COLOR_BLACK);
    terminal_write("\nRebooting Claudimon...\n");
    for (volatile uint32_t i = 0; i < 3000000; i++) __asm__("nop");
    power_reboot();
}

/* ============================================
   COMMAND PARSER
   ============================================ */
static void execute(char* cmd) {
    while(*cmd==' ') cmd++;
    if(*cmd=='\0') return;
    history_add(cmd);

    if      (k_strcmp(cmd,"help")==0)    cmd_help();
    else if (k_strcmp(cmd,"clear")==0)   cmd_clear();
    else if (k_strcmp(cmd,"logo")==0)    print_logo();
    else if (k_strcmp(cmd,"version")==0) cmd_version();
    else if (k_strcmp(cmd,"info")==0)    cmd_info();
    else if (k_strcmp(cmd,"mem")==0)     cmd_mem();
    else if (k_strcmp(cmd,"cpu")==0)     cmd_cpu();
    else if (k_strcmp(cmd,"color")==0)   cmd_color();
    else if (k_strcmp(cmd,"ls")==0)      fs_ls();
    else if (k_strcmp(cmd,"pwd")==0)     fs_pwd();
    else if (k_strcmp(cmd,"beep")==0)    speaker_beep();
    else if (k_strcmp(cmd,"jingle")==0)  speaker_boot_sound();
    else if (k_strcmp(cmd,"theme")==0)   theme_list();
    else if (k_strcmp(cmd,"history")==0) cmd_history();
    else if (k_strcmp(cmd,"keyboard")==0) cmd_keyboard("");
    else if (k_strcmp(cmd,"shutdown")==0) cmd_shutdown();
    else if (k_strcmp(cmd,"close")==0)    cmd_shutdown();
    else if (k_strcmp(cmd,"poweroff")==0) cmd_shutdown();
    else if (k_strcmp(cmd,"reboot")==0)   cmd_reboot();
    else if (k_strcmp(cmd,"restart")==0)  cmd_reboot();
    else if (k_strcmp(cmd,"echo")==0)    cmd_echo("");
    else if (k_strncmp(cmd,"echo ",5)==0)   cmd_echo(cmd+5);
    else if (k_strncmp(cmd,"mkdir ",6)==0)  fs_mkdir(cmd+6);
    else if (k_strncmp(cmd,"cd ",3)==0)     cmd_cd(cmd+3);
    else if (k_strcmp(cmd,"cd")==0)         cmd_cd("/");
    else if (k_strncmp(cmd,"cat ",4)==0)    fs_cat(cmd+4);
    else if (k_strncmp(cmd,"touch ",6)==0)  cmd_touch(cmd+6);
    else if (k_strncmp(cmd,"play ",5)==0)   cmd_play(cmd+5);
    else if (k_strncmp(cmd,"theme ",6)==0)  cmd_theme(cmd+6);
    else if (k_strncmp(cmd,"calc ",5)==0)   cmd_calc(cmd+5);
    else if (k_strncmp(cmd,"edit ",5)==0)   cmd_edit(cmd+5);
    else if (k_strncmp(cmd,"keyboard ",9)==0) cmd_keyboard(cmd+9);
    else {
        terminal_set_color(theme_error_color,VGA_COLOR_BLACK);
        terminal_write("Unknown command: '");terminal_write(cmd);terminal_write("'\n");
        speaker_error_sound();
    }
    terminal_set_color(theme_text_color,VGA_COLOR_BLACK);
}

/* ============================================
   PROMPT
   ============================================ */
static void print_prompt(void) {
    terminal_set_color(theme_prompt_color,VGA_COLOR_BLACK);
    terminal_write("claudimon");
    for(int i=0;i<path_depth;i++){terminal_write("/");terminal_write(path_segments[i]);}
    terminal_set_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK);
    terminal_write("> ");
    terminal_set_color(theme_text_color,VGA_COLOR_BLACK);
}

/* ============================================
   SHELL LOOP — with up arrow history + backspace
   ============================================ */
void shell_run(void) {
    char buf[CMD_BUFFER_SIZE];
    int  pos=0, len=0;
    int  hist_offset=0;  /* 0 = typing new cmd */

    theme_apply(0);
    print_logo();
    terminal_set_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK);
    terminal_write("Type 'help' to see available commands.\n\n");
    speaker_boot_sound();

    while(1) {
        print_prompt();
        pos=0; len=0; hist_offset=0;
        buf[0]='\0';

        while(1) {
            char c = keyboard_getchar();

            /* --- Enter --- */
            if(c=='\n') {
                terminal_putchar('\n');
                buf[len]='\0';
                execute(buf);
                break;

            /* --- Backspace (handles UK kbd: 0x0E scancode → '\b') --- */
            } else if(c=='\b') {
                if(pos>0 && len>0) {
                    /* Shift chars left */
                    for(int i=pos-1;i<len-1;i++) buf[i]=buf[i+1];
                    len--; pos--;
                    buf[len]='\0';
                    /* Redraw from cursor */
                    terminal_putchar('\b');
                    for(int i=pos;i<len;i++) terminal_putchar(buf[i]);
                    terminal_putchar(' ');
                    for(int i=pos;i<=len;i++) terminal_putchar('\b');
                }

            /* --- Up arrow: previous history entry --- */
            } else if(c=='\x1b') {  /* ESC sequence start */
                char c2=keyboard_getchar();
                char c3=keyboard_getchar();
                if(c2=='[' && c3=='A') {
                    /* Up arrow: previous history entry */
                    int next_offset=hist_offset+1;
                    const char* h=history_get(next_offset);
                    if(h) {
                        hist_offset=next_offset;
                        for(int i=0;i<len;i++) { terminal_putchar('\b'); }
                        for(int i=0;i<len;i++) terminal_putchar(' ');
                        for(int i=0;i<len;i++) { terminal_putchar('\b'); }
                        int i=0;
                        while(h[i]&&i<CMD_BUFFER_SIZE-1){buf[i]=h[i];terminal_putchar(h[i]);i++;}
                        len=i; pos=i; buf[len]='\0';
                    }
                } else if(c2=='[' && c3=='B') {
                    /* Down arrow: next history entry */
                    if(hist_offset>0) {
                        hist_offset--;
                        for(int i=0;i<len;i++) terminal_putchar('\b');
                        for(int i=0;i<len;i++) terminal_putchar(' ');
                        for(int i=0;i<len;i++) terminal_putchar('\b');
                        if(hist_offset==0) {
                            len=0; pos=0; buf[0]='\0';
                        } else {
                            const char* h=history_get(hist_offset);
                            int i=0;
                            while(h[i]&&i<CMD_BUFFER_SIZE-1){buf[i]=h[i];terminal_putchar(h[i]);i++;}
                            len=i; pos=i; buf[len]='\0';
                        }
                    }
                } else if(c2=='[' && c3=='S') {
                    /* Shift+Up: scroll the screen view up (view old output) */
                    terminal_scroll_up(3);
                } else if(c2=='[' && c3=='T') {
                    /* Shift+Down: scroll the screen view back down */
                    terminal_scroll_down(3);
                }

            /* --- Normal character --- */
            } else if(pos<CMD_BUFFER_SIZE-1) {
                /* Insert at pos */
                for(int i=len;i>pos;i--) buf[i]=buf[i-1];
                buf[pos]=c; len++; pos++;
                buf[len]='\0';
                /* Redraw from pos */
                for(int i=pos-1;i<len;i++) terminal_putchar(buf[i]);
                for(int i=pos;i<len;i++) terminal_putchar('\b');
                hist_offset=0;
            }
        }
    }
}

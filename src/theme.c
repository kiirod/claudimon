#include "theme.h"
#include "vga.h"

/* ============================================
   COLOUR THEMES
   ============================================ */

static theme_t themes[] = {
    /* 0: Classic green-on-black (default) */
    { VGA_COLOR_LIGHT_CYAN,  VGA_COLOR_LIGHT_GREEN,  VGA_COLOR_WHITE,
      VGA_COLOR_LIGHT_GREEN, VGA_COLOR_LIGHT_RED,    VGA_COLOR_LIGHT_CYAN,
      "classic" },
    /* 1: Amber — warm orange on black */
    { VGA_COLOR_LIGHT_BROWN, VGA_COLOR_LIGHT_BROWN,  VGA_COLOR_LIGHT_BROWN,
      VGA_COLOR_LIGHT_BROWN, VGA_COLOR_LIGHT_RED,    VGA_COLOR_WHITE,
      "amber" },
    /* 2: Ice — light blue tones */
    { VGA_COLOR_LIGHT_BLUE,  VGA_COLOR_LIGHT_CYAN,   VGA_COLOR_WHITE,
      VGA_COLOR_LIGHT_CYAN,  VGA_COLOR_LIGHT_RED,    VGA_COLOR_LIGHT_BLUE,
      "ice" },
    /* 3: Hacker — bright green only */
    { VGA_COLOR_GREEN,       VGA_COLOR_GREEN,         VGA_COLOR_GREEN,
      VGA_COLOR_LIGHT_GREEN, VGA_COLOR_LIGHT_RED,     VGA_COLOR_GREEN,
      "hacker" },
    /* 4: Rose — magenta/pink */
    { VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_WHITE,
      VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_LIGHT_RED,     VGA_COLOR_LIGHT_CYAN,
      "rose" },
};

#define NUM_THEMES 5
static int current_theme = 0;

/* Global colours used by shell and fs */
vga_color_t theme_banner_color;
vga_color_t theme_prompt_color;
vga_color_t theme_text_color;
vga_color_t theme_success_color;
vga_color_t theme_error_color;
vga_color_t theme_dir_color;

void theme_apply(int index) {
    if (index < 0 || index >= NUM_THEMES) {
        terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        terminal_write("theme: invalid theme index\n");
        return;
    }
    current_theme      = index;
    theme_t* t         = &themes[index];
    theme_banner_color  = t->banner;
    theme_prompt_color  = t->prompt;
    theme_text_color    = t->text;
    theme_success_color = t->success;
    theme_error_color   = t->error;
    theme_dir_color     = t->dir;

    terminal_set_color(t->success, VGA_COLOR_BLACK);
    terminal_write("Theme set to: ");
    terminal_write(t->name);
    terminal_write("\n");
}

void theme_list(void) {
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    terminal_write("\nAvailable themes:\n");
    for (int i = 0; i < NUM_THEMES; i++) {
        terminal_set_color(themes[i].prompt, VGA_COLOR_BLACK);
        if (i == current_theme) terminal_write("  * ");
        else                    terminal_write("    ");
        /* print index as char */
        char idx[2] = { '0' + i, '\0' };
        terminal_write(idx);
        terminal_write(" - ");
        terminal_write(themes[i].name);
        terminal_write("\n");
    }
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    terminal_write("\nUsage: theme <number>\n\n");
}

int theme_current(void) { return current_theme; }

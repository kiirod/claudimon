#ifndef THEME_H
#define THEME_H

#include "vga.h"

typedef struct {
    vga_color_t banner;
    vga_color_t prompt;
    vga_color_t text;
    vga_color_t success;
    vga_color_t error;
    vga_color_t dir;
    const char* name;
} theme_t;

void theme_apply(int index);
void theme_list(void);
int  theme_current(void);

/* Current theme colours — usable by any file that includes theme.h */
extern vga_color_t theme_banner_color;
extern vga_color_t theme_prompt_color;
extern vga_color_t theme_text_color;
extern vga_color_t theme_success_color;
extern vga_color_t theme_error_color;
extern vga_color_t theme_dir_color;

#endif

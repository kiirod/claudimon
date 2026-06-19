#include "vga.h"

/* ============================================
   VGA TEXT MODE DRIVER IMPLEMENTATION
   
   We write directly to memory address 0xB8000.
   This is "memory-mapped I/O" - writing to this
   address makes characters appear on screen!
   ============================================ */

/* Pointer to the VGA text buffer in memory */
static uint16_t* const VGA_BUFFER = (uint16_t*)0xB8000;

/* Terminal state */
static size_t   terminal_row;    /* Current cursor row (0-24) */
static size_t   terminal_col;    /* Current cursor column (0-79) */
static uint8_t  terminal_color;  /* Current text colour */

/* ---- Helper: write a single cell in the VGA buffer ---- */
static void terminal_put_entry_at(char c, uint8_t color, size_t col, size_t row) {
    /* Calculate index: row * width + col */
    const size_t index = row * VGA_WIDTH + col;
    VGA_BUFFER[index] = vga_entry(c, color);
}

/* ---- Clear the entire screen ---- */
void terminal_init(void) {
    terminal_row   = 0;
    terminal_col   = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);

    /* Fill every cell with a space (blank screen) */
    for (size_t row = 0; row < VGA_HEIGHT; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            terminal_put_entry_at(' ', terminal_color, col, row);
        }
    }
}

/* ---- Scroll screen up one line ---- */
static void terminal_scroll(void) {
    /* Move every row up by one */
    for (size_t row = 1; row < VGA_HEIGHT; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            size_t dst = (row - 1) * VGA_WIDTH + col;
            size_t src = row       * VGA_WIDTH + col;
            VGA_BUFFER[dst] = VGA_BUFFER[src];
        }
    }
    /* Clear the last row */
    for (size_t col = 0; col < VGA_WIDTH; col++) {
        terminal_put_entry_at(' ', terminal_color, col, VGA_HEIGHT - 1);
    }
    terminal_row = VGA_HEIGHT - 1;
}

/* ---- Change text colour ---- */
void terminal_set_color(vga_color_t fg, vga_color_t bg) {
    terminal_color = vga_entry_color(fg, bg);
}

/* ---- Print a single character ---- */
void terminal_putchar(char c) {
    if (c == '\n') {
        /* Newline: move to next row */
        terminal_col = 0;
        terminal_row++;
    } else if (c == '\r') {
        terminal_col = 0;
    } else if (c == '\t') {
        /* Tab: advance to next 4-column boundary */
        terminal_col = (terminal_col + 4) & ~3;
    } else {
        /* Normal character: write it to the buffer */
        terminal_put_entry_at(c, terminal_color, terminal_col, terminal_row);
        terminal_col++;
    }

    /* Wrap at end of line */
    if (terminal_col >= VGA_WIDTH) {
        terminal_col = 0;
        terminal_row++;
    }

    /* Scroll if we've gone past the last row */
    if (terminal_row >= VGA_HEIGHT) {
        terminal_scroll();
    }
}

/* ---- Print a null-terminated string ---- */
void terminal_write(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        terminal_putchar(str[i]);
    }
}

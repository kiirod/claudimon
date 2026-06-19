#include "keyboard.h"
#include "vga.h"

/* ============================================
   KEYBOARD DRIVER - POLLING MODE
   
   Instead of interrupts, we just check port 0x60
   directly. Much simpler and works reliably in QEMU.
   ============================================ */

static const char scancode_map[128] = {
    0,   0,  '1','2','3','4','5','6','7','8','9','0','-','=', 0,  '\t',
   'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
   'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
   'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void keyboard_init(void) {
    /* Nothing needed for polling mode */
}

char keyboard_getchar(void) {
    uint8_t scancode;
    uint8_t last = 0;

    while (1) {
        /* Wait until the keyboard buffer has data (bit 0 of status port) */
        while (!(inb(0x64) & 0x01));

        scancode = inb(0x60);

        /* Only act on key press (not release — release has bit 7 set) */
        if (scancode & 0x80) {
            last = 0;
            continue;
        }

        /* Ignore repeated scancodes */
        if (scancode == last) continue;
        last = scancode;

        char c = scancode_map[scancode];
        if (c != 0) return c;
    }
}

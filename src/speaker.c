#include "speaker.h"
#include "stdint.h"

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

#define PIT_BASE_FREQ 1193180

static void delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++)
        __asm__ volatile ("nop");
}

void speaker_play(uint32_t frequency) {
    if (frequency == 0) { speaker_stop(); return; }
    uint32_t divisor = PIT_BASE_FREQ / frequency;

    /* Programme PIT channel 2 — square wave mode */
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)(divisor & 0xFF));
    outb(0x42, (uint8_t)(divisor >> 8));

    /* Gate the speaker on (bits 0+1 of port 0x61) */
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp | 0x03);
}

void speaker_stop(void) {
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp & 0xFC);   /* clear bits 0 and 1 */
}

void speaker_beep(void) {
    speaker_play(880);
    delay(4000000);
    speaker_stop();
}

void speaker_error_sound(void) {
    speaker_play(300);
    delay(3000000);
    speaker_play(200);
    delay(3000000);
    speaker_stop();
}

void speaker_boot_sound(void) {
    uint32_t notes[] = { 523, 659, 784, 1047 };
    for (int i = 0; i < 4; i++) {
        speaker_play(notes[i]);
        delay(2500000);
        speaker_stop();
        delay(500000);
    }
}

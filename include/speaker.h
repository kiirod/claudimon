#ifndef SPEAKER_H
#define SPEAKER_H

#include "stdint.h"

/* Play a tone at the given frequency (Hz) for a short duration */
void speaker_play(uint32_t frequency);
void speaker_stop(void);
void speaker_beep(void);       /* short beep */
void speaker_boot_sound(void); /* startup jingle */
void speaker_error_sound(void);

#endif

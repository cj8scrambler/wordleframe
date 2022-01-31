#ifndef __AUDIO_H
#define __AUDIO_H

//#define _TIMERINTERRUPT_LOGLEVEL_     0
#include "ESP32_New_TimerInterrupt.h"

bool Play(unsigned char *data, unsigned long len,
          unsigned long sample_hz, int pin, bool repeat);
bool Stop(void);
bool IsComplete(void);

#endif // __AUDIO_H

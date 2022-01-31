#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <Adafruit_NeoPixel.h>
#include "wordleRecords.h"

class WordleDisplay
{
  public:
    WordleDisplay(int pin);
    void begin(void);
    bool show(state results[GUESSES][WORD_LEN], uint8_t score, bool animate);
    void clear(void);
    //bool show(state **results, int rows, int cols, bool animate);
  private:
    uint32_t state_to_color(state, uint8_t brightness);
    Adafruit_NeoPixel *pixels;
};

#endif // __DISPLAY_H

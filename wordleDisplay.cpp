#include "wordleDisplay.h"

WordleDisplay::WordleDisplay(int pin)
{
  pixels = new Adafruit_NeoPixel(GUESSES*WORD_LEN, pin, NEO_GRB);
}

void WordleDisplay::begin(void)
{
  pixels->begin();
  clear();
}

bool WordleDisplay::show(state results[GUESSES][WORD_LEN], uint8_t score, bool animate)
{
  int r,c;

  if (animate)
  {
    pixels->fill(0x0);
    pixels->show();
  }

  for (r = 0; r < score; r++)
  {
    for (c = 0; c < WORD_LEN; c++)
    {
      int i = r * WORD_LEN + c;
      uint32_t color;
      if (animate)
      {
        int b;
        for (b = 0; b < 255; b += 8)
        {
          color = state_to_color(results[r][c], b);
          pixels->setPixelColor(i, color);
          pixels->show();
          delay(5); // animation rate
        }
        delay(20); // letter delay
      } else {
        color = state_to_color(results[r][c], 255);
        pixels->setPixelColor(i, color);
      }
    }
    if (animate)
    {
      delay(200); // line delay
    }
  }

  if (!animate)
  {
    pixels->show();
  }
  return true;
}

void WordleDisplay::clear(void)
{
  pixels->clear();
  pixels->show();
}

uint32_t WordleDisplay::state_to_color(state s, uint8_t brightness)
{
  uint32_t color = 0; // Black

  switch(s)
  {
    case CORRECT:
      color = Adafruit_NeoPixel::ColorHSV(21845, 255, brightness); // Green 
      break;
    case CLOSE:
      color = Adafruit_NeoPixel::ColorHSV(10923, 255, brightness); // Yellow 
      break;
    case WRONG:
      color = Adafruit_NeoPixel::ColorHSV(0, 0, brightness/2); // 50% grey
      break;
    case UNUSED:
      color = Adafruit_NeoPixel::ColorHSV(0, 0, 0); // black
      break;
    default:
      Serial.printf("Unkown state encountered (%d)\r\n", s);
  }
  return color;
}

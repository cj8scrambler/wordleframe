#include <Arduino.h>
#include "audio.h"
#include "ESP32_New_TimerInterrupt.h"

ESP32Timer *timer = NULL;

typedef struct audioPbData {
  unsigned char *data;
  unsigned long len;
  unsigned long offset;
  bool repeat;
  bool playing;
} audioPbData;

bool IRAM_ATTR PlayFrameCallback(void * timerNo);

volatile audioPbData g_pbdata = {0};
int g_pcm_pin;

bool Play(unsigned char *data, unsigned long len, unsigned long sample_hz, int pin, bool repeat)
{
  if (g_pbdata.playing)
  {
    Serial.println("Error: Audio playback in progress\n");
    return false;
  }

  if (!timer)
  {
    timer = new ESP32Timer(0);
    if (!timer)
    {
      Serial.println("Unable to allocate timer 0");
      return false;
    }
  }

  g_pbdata.offset  = 0;
  g_pbdata.playing = true;
  g_pbdata.repeat = repeat;
  g_pbdata.data = data;
  g_pbdata.len = len;
  g_pcm_pin = pin;

  if (! timer->setFrequency(sample_hz, PlayFrameCallback))
  {
    Serial.println(F("Playback failure. Select another frequency or check timer conflict"));
    return false;
  }

  return true;
}

bool PlayFrameCallback(void * timerNo)
{ 
  if (g_pbdata.playing)
  {
    dacWrite(g_pcm_pin, g_pbdata.data[g_pbdata.offset++]);
    if (g_pbdata.offset >= g_pbdata.len)
      if (g_pbdata.repeat)
        g_pbdata.offset = 0;
      else
      {
        g_pbdata.playing = false;
        timer->detachInterrupt();
      }
  }

  return true;
}

bool Stop(void)
{
  if (timer)
  {
    timer->detachInterrupt();
    g_pbdata.playing = false;
    return true;
  }
  return false;
}

bool IsComplete(void)
{
  return (! g_pbdata.playing);
}

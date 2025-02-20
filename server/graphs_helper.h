#pragma once
#include <Arduino.h>

// Server configuration
const int FADE_LENGTH = 30;

// Graph arrays for wake-up components
float graph_light[FADE_LENGTH];
float graph_music[FADE_LENGTH];
float graph_backup[FADE_LENGTH];
#if ENABLE_WAKE_COFFEE
float graph_coffee[FADE_LENGTH];
#endif

void computeGraph_linearFade(int delay_time, int fade_time, float *values)
{
  for (int i = 0; i < FADE_LENGTH; i++)
  {
    if (i <= delay_time)
      values[i] = 0;
    else if (i <= (delay_time + fade_time))
      values[i] = float(i - delay_time) / float(fade_time);
    else
      values[i] = 1.0;

    values[i] *= 100.0;
  }
}

void computeGraph_step(int delay_time, float *values)
{
  for (int i = 0; i < FADE_LENGTH; i++)
  {
    if (i < delay_time)
      values[i] = 0;
    else
      values[i] = 1.0;

    values[i] *= 100.0;
  }
}

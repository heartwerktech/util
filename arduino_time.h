#pragma once
#include <Arduino.h>
#include <elapsedMillis.h>

using lpsd_ms = elapsedMillis;
using time_ms = int32_t;
using time_s = int32_t;
using time_min = int32_t;

namespace util
{

    inline time_ms now_ms()
    {
        return millis();
    }

    inline time_ms since_ms(time_ms event_ms)
    {
        return now_ms() - event_ms;
    }

    inline float getRelative(time_ms timer, time_ms period)
    {
        if (period < timer)
        {
            printf("ERROR in util::getRelative: period %d < timer %d\n", period, timer);
            // return 1;
        }
        return constrain(float(timer) / float(period), 0, 1);
    }

    inline float getRelative(lpsd_ms timer, time_ms period)
    {
        return getRelative(static_cast<time_ms>(timer), period);
    }
}
#pragma once
#include <Arduino.h>

#include "../util.h"

#define USE_ESP32_LEDC 1

// Single definition of PWM constants
#define PWM_FREQUENCY 20000 // Updated to match h_bridge_driver.h value
const int PWM_RANGE_BITS = 8;
const int PWM_RANGE = (1 << PWM_RANGE_BITS);

class PWM_Driver
{
public:
    PWM_Driver()
    {
    }

    PWM_Driver(uint8_t pin)
    {
        setup(pin);
    }

    void setup(uint8_t pin)
    {
        Serial.printf("setup PWM_Driver pin %d\n", pin);

        _pin = pin;
        pinMode(_pin, OUTPUT);

#if USE_ESP32_LEDC
        ledcAttach(_pin, PWM_FREQUENCY, PWM_RANGE_BITS);
#else
        // TODO why does this not work anymore ?
        // analogWriteFrequency(PWM_FREQUENCY);
        // analogWriteRange(PWM_RANGE);
#endif

        set(0);
    }

    // 0 to 1
    void set(float percentage)
    {
        _current = percentage;

        int pwm = util::mapConstrainf(fabs(_current), 0, 1, 0, float(PWM_RANGE - 1));
#if USE_ESP32_LEDC
        ledcWrite(_pin, pwm);
#else
        analogWrite(_pin, pwm);
#endif
    }

    float get() { return _current; }

private:
    float _current = 0;
    uint8_t _pin;
};

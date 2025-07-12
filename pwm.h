#pragma once
#include <Arduino.h>

#include "util.h"

// Single definition of PWM constants
#define PWM_FREQUENCY 20000 // Updated to match h_bridge_driver.h value
const int PWM_RANGE_BITS = 8;
const int PWM_RANGE      = (1 << PWM_RANGE_BITS);

// works for esp32 and esp8266:
class PWM_Driver
{
public:
    PWM_Driver() {}

    PWM_Driver(uint8_t pin) { setup(pin); }

    void setup(uint8_t pin)
    {
        Serial.printf("setup PWM_Driver pin %d\n", pin);

        _pin = pin;
        pinMode(_pin, OUTPUT);

#ifdef ARDUINO_ARCH_ESP32
        // ESP32 doesn't have analogWriteFreq/analogWriteResolution in the same way
        // Use ledc functions instead for ESP32
        _channel = _getNextChannel();
        ledcAttach(_pin, PWM_FREQUENCY, PWM_RANGE_BITS);
#else
        analogWriteFreq(PWM_FREQUENCY);
        analogWriteResolution(PWM_RANGE_BITS);
#endif

        set(0);
    }

    // 0 to 1
    void set(float percentage)
    {
        _current = percentage;

        int pwm = util::mapConstrainf(fabs(_current), 0, 1, 0, float(PWM_RANGE - 1));
#ifdef ARDUINO_ARCH_ESP32
        ledcWrite(_pin, pwm);
#else
        analogWrite(_pin, pwm);
#endif
    }

    float get() { return _current; }

private:
    float   _current = 0;
    uint8_t _pin;
    uint8_t _channel = 0;

    static uint8_t _getNextChannel()
    {
        static uint8_t nextChannel = 0;
        return nextChannel++;
    }
};

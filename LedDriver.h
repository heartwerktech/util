
#pragma once
#include <Arduino.h>
#include <elapsedMillis.h>
#include "pwm.h"

#include "util.h"

class LedDriver
{
public:
    LedDriver(uint8_t pin)
        : _pwm(pin)
    {
    }

    void setup() {}

    void loop()
    {
        if (_since_loop > 2)
        {
            _since_loop = 0;
            apply();
        }
    }

    void setFilterValue(float value) { _filterValue = value; }

    void set(float percentage) // 0-1.0f
    {
        _target = constrain(percentage, 0.0f, 1.0f);
    }

    void setDirectly(float percentage)
    {
        set(percentage);
        _current = _target;
        apply();
    }

    void apply()
    {
        simpleFilterf(_current, _target, _filterValue);
        
        _pwm.set(_current);
    }

private:
    elapsedMillis _since_loop  = 0;

    float _target  = 0;
    float _current = 0;

    float _filterValue = 0.02f;

    PWM_Driver _pwm;
};

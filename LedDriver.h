#pragma once
#include <Arduino.h>
#include <elapsedMillis.h>
#include <functional>
#include "pwm.h"

#include "util.h"

class LedDriver;
using LedDrivers = std::vector<LedDriver>;

class LedDriver
{
public:
    LedDriver(uint8_t pin)
        : _pwm(pin)
    {
    }

    void setup() { setGamma(2.8f); }

    void loop()
    {
        if (_since_loop > 2)
        {
            _since_loop = 0;
            apply();
        }
    }

    void setFilterValue(float value) { _filterValue = value; }

    void setGamma(float gamma) { _gamma = gamma; }

    float applyGamma(float value) { return powf(constrain(value, 0.0f, 1.0f), _gamma); }

    void set(float percentage) // 0-1.0f
    {
        _since_set = 0;
        // printf("map %2.2f to %2.2f\n", percentage, applyGamma(percentage));

        _target = constrain(percentage, 0.0f, 1.0f);
    }

    void toggle(bool state)
    {
        if (!state) // off
        {
            _last_target = _target;
            set(0);
        }
        else // on
        {
            set(_last_target);
        }
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

#if 0
        if (_since_set > 2000)
        {
            _since_set = 0;
            if (_target > 0)
            {
                set(_target * 0.9f);
                if (onSelfUpdate)
                    onSelfUpdate();
            }
        }
#endif

        float correctedValue = applyGamma(_current);
        _pwm.set(correctedValue);
    }

    float get() const { return _target; }

    void setOnSelfUpdate(std::function<void()> callback) { onSelfUpdate = callback; }

private:
    std::function<void()> onSelfUpdate = nullptr;

    elapsedMillis _since_loop = 0;
    elapsedMillis _since_set  = 0;

    float _target  = 0;
    float _current = 0;
    float _gamma   = 2.8f; // Default gamma correction value

    float _filterValue = 0.02f;

    float _last_target = 1.0f;

    PWM_Driver _pwm;
};

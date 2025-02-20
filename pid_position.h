#pragma once
#include "pid.h"
#include "util.h"
#include <elapsedMillis.h>

using namespace util;

class PID_position : public PID
{
public:

    float process(float in)
    {
        float out = PID::process(in);

        // TODO mechanism to switch off when position is reached.
        if (fabs(_error) < _target_range)
        {
            if (!_in_target_range)
            {
                _in_target_range = true;
                _since_in_target_range = 0;
            }
            if (_since_in_target_range > 50)
            {
                _amplitude_factor = 1.0 - mapConstrainf(_since_in_target_range, 50, 300, 0, 1);
                // setVoltageAmplitude(fade_factor);
                out *= _amplitude_factor;
            }
            if (_since_in_target_range > _time_stable)
            {
                out = 0;
                _amplitude_factor = 0.0;
            }
        }
        else
            _in_target_range = false;

        return out;
    }

    bool positionReached()
    {
        bool condition = (_in_target_range && _since_in_target_range > _time_stable);

        if (!_position_reached && condition)
        {
            _position_reached = true;
            return true;
        }
        else
            return false;
    }
    
    float amplitudeFactor() { return _amplitude_factor; }

    void setTarget(float t)
    {
        _amplitude_factor = 1.0;
        _position_reached = false;
        _in_target_range = false;
        PID::setTarget(t);
    }

    void setParams_StableInRange(float start_fade, float time_stable, float target_range)
    {
        _time_start_fade = start_fade;
        _time_stable = time_stable;
        _target_range = target_range;
    }

    // ====================================================================
    // parameters
    time_ms _time_start_fade = 60; 
    time_ms _time_stable = 300;
    float _target_range = 0.15;

    // ====================================================================
    float _amplitude_factor = 1.0; // used to fade out after position is reached.

    elapsedMillis _since_in_target_range = 0;
    bool _in_target_range = false;
    bool _position_reached = false; // for bool getter

protected:
};

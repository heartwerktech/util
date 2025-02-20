#pragma once
#include "basics.h"

using namespace util;

class PID
{
public:
    float process(float in)
    {
        _input = in;
        _error = _target - _input;
        float delta = _input - _input_z1;
        _output = process_internal(delta);

        _input_z1 = _input;
        
        return _output;
    }

    float process(float in, float t)
    {
        setTarget(t);
        return process(in);
    }

    // TODO maybe subclass this.
    float process_unwrap(float in)
    {
        _input = in;
        _error = wrap(_target - _input, -0.5f, 0.5f);
        float delta = wrap(_input - _input_z1, -0.5f, 0.5f);
        _output = process_internal(delta);

        _input_z1 = _input;
        
        return _output;
    }

    float process_internal(float delta_in)
    {
        _output_ki += _ki * _error / float(_processRate);
        _output_ki = clipf(_output_ki, -20, 20);

        _output = 0;
        _output += _kp * _error;
        _output += _output_ki;
#if 1
        static float delta = 0;
        delta += (delta_in - delta) * 0.1; // super simple quick filter
        delta_in = delta;
#endif
        _output += _kd * delta_in * float(_processRate) / 1000.0;

        _input_z1 = _input;

        return _output;
    }

    void setParams(float Kp, float Ki, float Kd)
    {
        _kp = Kp;
        _ki = Ki;
        _kd = Kd;
    }
    void setSampleRate(int rate) { _processRate = rate; }

    void reset()
    {
        _output_ki = 0;
        _input_z1 = _input;
    }

    void setTarget(float t)
    {
        _target = t;
        _error = 1.0;
    }

public:
    float _target = 0;
    float _error = 0;

    float _kp; // * (P)roportional Tuning Parameter
    float _ki; // * (I)ntegral Tuning Parameter
    float _kd; // * (D)erivative Tuning Parameter

    float _input = 0;
    float _output = 0;

    float _output_ki, _input_z1;

    int _processRate = 1000; // = sample rate
};

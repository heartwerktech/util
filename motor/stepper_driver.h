#pragma once

#include <Arduino.h>
#include <AccelStepper.h>

// # REQUIREMENTS:

// lib_deps =
// 	waspinator/AccelStepper@^1.64

// Using eg. L298N as dual h_bridge

struct Stepper_Driver : public AccelStepper
{

public:
    Stepper_Driver(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4) : AccelStepper(AccelStepper::FULL4WIRE, in1, in2, in3, in4)
    {
        _pins.in1 = in1;
        _pins.in2 = in2;
        _pins.in3 = in3;
        _pins.in4 = in4;
    }

    void setup()
    {
        AccelStepper::setMaxSpeed(600);
        AccelStepper::setAcceleration(2000);

        AccelStepper::disableOutputs();
    }

    void set(int steps)
    {
        enableOutputs();
        AccelStepper::moveTo(steps);
        _turnOffAfterMove = true;

        _reachedGoal = false;
        _newSetGoal = true;
    }

    void setOffset(int steps_relative)
    {
        enableOutputs();
        AccelStepper::move(steps_relative);
        _turnOffAfterMove = true;

        _reachedGoal = false;
        _newSetGoal = true;
    }

    void loop()
    {
        if (distanceToGo() == 0)
        {
            if (_newSetGoal)
                _reachedGoal = true;

            if (_turnOffAfterMove)
                disableOutputs();
        }

        run();
    }

    bool reachedGoal()
    {
        if (!_newSetGoal)
            return false;

        if (_reachedGoal)
        {
            _reachedGoal = false;
            _newSetGoal = false;
            return true;
        }
        return false;
    }



public:
    bool _newSetGoal = false;
    bool _reachedGoal = false;
    bool _turnOffAfterMove = false;
    struct PINS
    {
        uint8_t in1;
        uint8_t in2;
        uint8_t in3; 
        uint8_t in4;
    };

    PINS _pins;

};

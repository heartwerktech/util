#pragma once

/**
 * @file motor_driver_base.h
 * @brief This file contains the declaration of the MotorDriverBase class.
 */

class MotorDriverBase
{
public:
    MotorDriverBase() {}

    virtual void setup();
    virtual void loop();
    virtual void setSpeed(float percentage);
};

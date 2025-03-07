/**
 * @file H_Bridge_Driver.h
 *
 * This class provides a basic PWM driver for an H-bridge, enabling speed and direction control of a motor.
 * Based on Ardunio.h
 *
 * Usage:
 * 1. #include "h_bridge_driver.h"
 * 2. Create an instance of the H_Bridge_Driver class, providing the control pin numbers as parameters to the constructor.
 * 3. Call the begin() function to initialize the control pins.
 * 4. Set the desired speed using the setSpeed() function, providing a percentage value between -1.0 and 1.0.
 * 5. Call the loop() function periodically to update the speed and apply it to the motor.
 *
 * Example:
 * @code
 * #include "h_bridge_driver.h"
 *
 * H_Bridge_Driver motor(9, 10); // Create an instance of H_Bridge_Driver with control pins 9 and 10
 *
 * void setup() {
 *   motor.begin(); // Initialize the control pins
 * }
 *
 * void loop() {
 *   motor.setSpeed(0.5); // Set the speed to 50%
 *   motor.loop(); // apply update speed (run/call at least with 200Hz)
 *   do other stuff
 * }
 * @endcode
 */

// TODO: why not make this a more generic H-Bridge driver class or at least inherit from it ?!

#pragma once
#include <Arduino.h>
#include "motor_driver_base.h"
#include <elapsedMillis.h>
#include "pwm.h"

#include "../util.h"

#define ESP32C6 1 // TODO move this somewhere else

class H_Bridge_Driver
{
public:
    H_Bridge_Driver(uint8_t pin1, uint8_t pin2) : _pwm1(pin1), _pwm2(pin2)
    {
    }

    void setup()
    {
        }

    void setFilterValue(float value)
    {
        _filterValue = value;
    }

    void begin() // legacy
    {
        setup();
    }

    void loop()
    {
        if (_since_loop > 2)
        {
            _since_loop = 0;
            applySpeed();
        }
    }

    void setPowerPercentage(float percentage)
    {
        _power_factor = util::clipf(percentage, 0, 1);
    }

    void setSpeed(float percentage)
    {
        set(percentage);
    }

    void set(float percentage)
    {
        // printf("H_Bridge_Driver::setSpeed: %2.2f\n", percentage);
        _target = constrain(percentage, -1.0f, 1.0f);
    }

    void setDirectly(float percentage)
    {
        _current = constrain(percentage, -1.0f, 1.0f);
        _target = _current;
        applySpeed();
    }

    void applySpeed()
    {
        simpleFilterf(_current, _target, _filterValue);
        float output = (_invert_dir ? -1 : 1) * _current;

        if (output > 0)
        {
            _pwm1.set(fabs(output));
            _pwm2.set(0);
        }
        else
        {
            _pwm1.set(0);
            _pwm2.set(fabs(output));
        }

        if (since_update > 100)
        {
            since_update = 0;
        }
    }

    float get() { return _current; }

    float getActual() { return get(); }
    float getTarget() { return _target; }

public:
    elapsedMillis since_update = 0;

    // uint8_t feedback_speed_pin;
    // int counter_speed_pulse = 0;

    bool _invert_dir = false;

private:
    float _power_factor = 1.0f;
    elapsedMillis _since_loop = 0;
    elapsedMillis _since_count = 0;
    elapsedMillis _since_reverse = 0;

    float _target = 0;
    float _current = 0;

    float _filterValue = 0.02f;

    PWM_Driver _pwm1;
    PWM_Driver _pwm2;
};

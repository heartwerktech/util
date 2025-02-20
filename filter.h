#pragma once
#include <Arduino.h>

static void simpleFilterf(float &value, float target, float filterFactor = 0.2, float lastStep = 0.01)
{
    if (value == target)
        return;
    else if (abs(value - target) <= lastStep)
        value = target;
    else
        value = value * (1.0 - filterFactor) + target * filterFactor; // TODO maybe better implementation for this
}

static int simpleFilter(int &value, int target, float filterFactor)
{
    int maxStep = float(1.0 / filterFactor) + 1;
    
    if (abs(value - target) <= maxStep)
        value = target;
    else
        value = value * (1.0 - filterFactor) + target * filterFactor;

    return value;
}

static void simpleFilterWrapf(float &value, float target, float filterFactor = 0.2, float lastStep = 0.01)
{
    if (value == target)
        return;
    else if (abs(value - target) <= lastStep)
        value = target;
    else
    {
        float diff = target - value;
        if (diff > 0.5)
            diff -= 1;
        else if (diff < -0.5)
            diff += 1;
        value += diff * filterFactor;
    }
}
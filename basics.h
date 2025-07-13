#pragma once

#include <cmath>

namespace util {

#define MIN_TO_MS 60000

// int sign(float value) { return (value > 0) - (value < 0); }

template <typename T>
static T map(T value, float inMin, float inMax, float outMin, float outMax)
{
    return (value - inMin) / (inMax - inMin) * (outMax - outMin) + outMin;
}

inline float clipf(float value, float low, float high)
{
    if (value < low)
        return low;
    if (value > high)
        return high;
    return value;
}

template <typename T>
T clip(T value, T low, T high) {
    if (value < low)
        return low;
    if (value > high)
        return high;
    return value;
}

#ifndef ARDUINO 
template <typename T>
T clip(T value, T low, T high) {
    if (value < low)
        return low;
    if (value > high)
        return high;
    return value;
}
#endif


inline float mapf(float value, float fromLow, float fromHigh, float toLow, float toHigh) {
    return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}


inline float mapConstrainf(float value, float fromLow, float fromHigh, float toLow, float toHigh) {
    return clipf(mapf(value, fromLow, fromHigh, toLow, toHigh), toLow, toHigh);
}

/*
// NOT FULLY TESTED yet
float mapExpf(float value, float fromLow, float fromHigh, float toLow, float toHigh, float curveFactor=2.0) {
    // Handle edge case where the input range is zero
    if (fromLow == fromHigh) {
        return toLow; // Map to the lower bound of the target range
    }

    // Ensure the curve factor is valid
    if (curveFactor <= 0) {
        return NAN; // Invalid curve factor
    }

    // Shift the input range to make it positive if necessary
    float shift = (fromLow <= 0) ? -fromLow + 1 : 0;

    // Apply the shift to the input values
    float shiftedValue = value + shift;
    float shiftedFromLow = fromLow + shift;
    float shiftedFromHigh = fromHigh + shift;

    // Normalize the shifted value to the range [0, 1]
    float normalized = (shiftedValue - shiftedFromLow) / (shiftedFromHigh - shiftedFromLow);
    normalized = clip(normalized, 0.0f, 1.0f); // Clamp to [0, 1]

    // Apply exponential curve
    float expValue = pow(normalized, curveFactor);

    // Map to the target range
    return expValue * (toHigh - toLow) + toLow;
}
*/

/*

  printf("--------------------\n");
  for (int i=0;i<100;i+=5)
    printf("Mapping Log: %d -> %f\n", i, util::mapExpConstraintf(i, 0, 100, 0, 1));
  printf("--------------------\n");
  for (int i=0;i<100;i+=5)
    printf("Mapping Log: %d -> %f\n", i, util::mapExpConstraintf(i, 0, 100, 0, 100));
  printf("--------------------\n");
  
  */

/*
float mapExpConstraintf(float value, float fromLow, float fromHigh, float toLow, float toHigh, float curveFactor=2.0) {
    return constrain(mapExpf(value, fromLow, fromHigh, toLow, toHigh, curveFactor), toLow, toHigh);
}
*/

// NOT FULLY TESTED yet
inline float mapLogf(float value, float fromLow, float fromHigh, float toLow, float toHigh) {
    // Handle the edge case where the input range is zero
    if (fromLow == fromHigh) {
        return toLow; // Return the lower bound of the target range if the input range is invalid
    }

    // Shift the input range to make it positive if necessary
    float shift = (fromLow <= 0) ? -fromLow + 1 : 0;

    // Shifted values for logarithmic calculation
    float shiftedValue = value + shift;
    float shiftedFromLow = fromLow + shift;
    float shiftedFromHigh = fromHigh + shift;

    // Map `value` to a logarithmic scale
    float logValue = (log(shiftedValue) - log(shiftedFromLow)) / (log(shiftedFromHigh) - log(shiftedFromLow));

    // Map the logarithmic value to the desired range
    return logValue * (toHigh - toLow) + toLow;
}

inline float mapLogConstrainf(float value, float fromLow, float fromHigh, float toLow, float toHigh) {
    return clip(mapLogf(value, fromLow, fromHigh, toLow, toHigh), toLow, toHigh);
}

inline float mapConstrainf_withCenter(float value, float fromLow, float fromCenter, float fromHigh, float toLow, float toHigh)
{
    if (value == fromCenter)
        return (toLow + (toHigh - toLow) / 2.0f);
    else if (value < fromCenter)
        return mapConstrainf(value, fromLow, fromCenter, toLow, 0);
    else
        return mapConstrainf(value, fromCenter, fromHigh, 0, toHigh);
}


/**
 * @brief Applies center hysteresis to the given value.
 * 
 * @param value The input value to apply center hysteresis to. (expects a value from -1 to +1 )
 * @param deadzone_width The width of the deadzone around zero.
 * @return The mapped value within the range from -1 to +1.
 */
inline float centerHysteris(float value, float deadzone_width)
{
    if (value > deadzone_width)
        return mapf(value, deadzone_width, 1, 0, 1);
    else if (value < -deadzone_width)
        return mapf(value, -deadzone_width, -1, 0, -1);
    else
        return 0;
}

template <typename T>
T wrap(T value, T low, T high) {
    T range = high - low;
    while (value < low)
        value += range;
    while (value >= high)
        value -= range;
    return value;
}

inline float wrapf(float value, float low, float high) {
    return wrap(value, low, high);
}

inline float normf(float value, float low, float high) {
    return clipf((value - low) / (high - low), 0, 1);
    // return (value - low) / (high - low);
}


inline float convert_zero_zone(float value, float zero_zone)
{
    float sign = value > 0 ? 1 : -1;

    value = fabs(value);
    value -= zero_zone;
    value /= (1.0 - zero_zone);

    value = clipf(value, 0, 1);

    return value * sign;
}



} // namespace util
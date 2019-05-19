#pragma once
//#include <cmath>
#include <math.h>
#include <stdint.h>

/*
 * Constrain a value to be within the range: low and high
 */
template <typename T>
T constrain_value(const T amt, const T low, const T high);



inline float constrain_float(const float amt, const float low, const float high)
{
    return constrain_value(amt, low, high);
}

inline int16_t constrain_int16(const int16_t amt, const int16_t low, const int16_t high)
{
    return constrain_value(amt, low, high);
}

inline int32_t constrain_int32(const int32_t amt, const int32_t low, const int32_t high)
{
    return constrain_value(amt, low, high);
}


template <typename T>
inline T constrain_value(const T amt, const T low, const T high)
{
    // the check for NaN as a float prevents propagation of floating point
    // errors through any function that uses constrain_value(). The normal
    // float semantics already handle -Inf and +Inf
    if (isnan(amt)) {
        return (low + high) / 2;
    }

    if (amt < low) {
        return low;
    }

    if (amt > high) {
        return high;
    }

    return amt;
}
template int constrain_value<int>(const int amt, const int low, const int high);
template long constrain_value<long>(const long amt, const long low, const long high);
template short constrain_value<short>(const short amt, const short low, const short high);
template float constrain_value<float>(const float amt, const float low, const float high);
template double constrain_value<double>(const double amt, const double low, const double high);


#ifndef DURATION_AND_UNIT_HPP

#include "TL6WL.h"
#include <string>

#define DURATION_AND_UNIT_HPP

struct DurationAndUnit {
    ViUInt32 duration;
    std::string unit;
};

/// <summary>
///  Given a duration in microseconds, find the most appropriate unit (us or ms).
/// </summary>
/// <param name="duration_us">The duration in us</param>
/// <returns></returns>
inline DurationAndUnit findDurationAndUnit(ViUInt32 duration_us) {
    DurationAndUnit result;
    if (duration_us % 1000 == 0) {
        result.duration = duration_us / 1000;
        result.unit = "ms";
    }
    else {
        result.duration = duration_us;
        result.unit = "us";
    }
    return result;
}

#endif  // DURATION_AND_UNIT_HPP
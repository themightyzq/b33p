#pragma once

#include <cmath>

namespace B33p
{
    // Snap an arbitrary time value to the nearest multiple of
    // gridSeconds. gridSeconds <= 0 disables snap and returns
    // seconds unchanged — that's the convention the UI's "Off"
    // grid-size dropdown uses.
    //
    // Header-only: trivial enough that the round-trip-through-cpp
    // overhead would dwarf the function itself.
    inline double snapToGrid(double seconds, double gridSeconds) noexcept
    {
        if (gridSeconds <= 0.0)
            return seconds;
        return std::round(seconds / gridSeconds) * gridSeconds;
    }
}

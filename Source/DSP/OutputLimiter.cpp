#include "OutputLimiter.h"

#include <cmath>

namespace B33p
{
    void OutputLimiter::prepare(double /*sampleRate*/)
    {
        prepared = true;
    }

    void OutputLimiter::reset()
    {
        // Intentionally empty — memoryless soft-clip has no state.
    }

    float OutputLimiter::processSample(float input)
    {
        if (! prepared)
            return 0.0f;

        const float magnitude = std::fabs(input);
        if (magnitude <= kKnee)
            return input;   // clean signal below the knee passes untouched

        // Exponential soft-knee above the threshold. At |x| = kKnee the
        // slope is 1 (continuous with the identity region); as |x| grows
        // the output approaches `kCeiling` (rounding up to it once the
        // exponential underflows), so the result is always bounded in
        // [-kCeiling, kCeiling] — and kCeiling itself sits below 0 dBFS.
        const float range  = kCeiling - kKnee;
        const float shaped = kKnee + range * (1.0f - std::exp(-(magnitude - kKnee) / range));
        return input < 0.0f ? -shaped : shaped;
    }
}

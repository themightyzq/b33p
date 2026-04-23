#include "Distortion.h"

#include <algorithm>
#include <cmath>

namespace B33p
{
    void Distortion::prepare(double /*sampleRate*/)
    {
        prepared = true;
    }

    void Distortion::reset()
    {
        // Intentionally empty — memoryless waveshaper has no state.
    }

    void Distortion::setDrive(float newDrive)
    {
        drive = std::clamp(newDrive, 0.1f, 100.0f);
    }

    float Distortion::processSample(float input)
    {
        if (! prepared)
            return 0.0f;

        return std::tanh(drive * input);
    }
}

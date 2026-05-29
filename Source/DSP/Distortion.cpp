#include "Distortion.h"

#include <algorithm>
#include <cmath>

namespace B33p
{
    void Distortion::prepare(double sampleRate)
    {
        prepared = true;
        driveSmoother.reset(sampleRate, 0.020);   // 20 ms ramp
        driveSmoother.setCurrentAndTargetValue(drive);
        firstSetAfterPrepare = true;
    }

    void Distortion::reset()
    {
        // Intentionally empty — memoryless waveshaper has no state.
        // (The smoother's current value is left alone so a reset on a
        // ringing voice doesn't snap drive to its target instantly.)
    }

    void Distortion::setDrive(float newDrive)
    {
        // Clamp first so the smoother target is never out-of-range.
        // First set after prepare snaps (matches test pattern + the
        // initial pushParametersToLane); later sets ramp.
        const float clamped = std::clamp(newDrive, 0.1f, 100.0f);
        if (firstSetAfterPrepare)
            driveSmoother.setCurrentAndTargetValue(clamped);
        else
            driveSmoother.setTargetValue(clamped);
    }

    float Distortion::processSample(float input)
    {
        if (! prepared)
            return 0.0f;

        drive = driveSmoother.getNextValue();
        firstSetAfterPrepare = false;
        return std::tanh(drive * input);
    }
}

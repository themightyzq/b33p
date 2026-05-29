#include "Distortion.h"

#include "SmoothingHelpers.h"

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
        // Intentionally clears no state — memoryless waveshaper. The
        // smoother's current value is left alone so a reset on a
        // ringing voice doesn't snap drive to its target instantly;
        // re-arm the snap-on-first-set flag so the next setX after a
        // reset() snaps to its target (matches prepare()'s contract).
        firstSetAfterPrepare = true;
    }

    void Distortion::setDrive(float newDrive)
    {
        // Clamp first so the smoother target is never out-of-range.
        // First set after prepare snaps (matches test pattern + the
        // initial pushParametersToLane); later sets ramp.
        setSmoothedTarget(driveSmoother,
                          std::clamp(newDrive, 0.1f, 100.0f),
                          firstSetAfterPrepare);
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

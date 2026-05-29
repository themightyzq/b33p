#include "Bitcrush.h"

#include <algorithm>
#include <cmath>

namespace B33p
{
    void Bitcrush::prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        prepared   = true;
        reset();
        recomputeQuantStep();
        recomputePhaseIncrement();
        bitDepthSmoother.reset(sampleRate, 0.030);     // 30 ms
        targetHzSmoother.reset(sampleRate, 0.030);
        bitDepthSmoother.setCurrentAndTargetValue(bitDepth);
        targetHzSmoother.setCurrentAndTargetValue(targetHz);
        firstSetAfterPrepare = true;
    }

    void Bitcrush::reset()
    {
        phase      = 1.0;
        heldSample = 0.0f;
    }

    void Bitcrush::setBitDepth(float bits)
    {
        const float clamped = std::clamp(bits, 1.0f, 16.0f);
        if (firstSetAfterPrepare)
            bitDepthSmoother.setCurrentAndTargetValue(clamped);
        else
            bitDepthSmoother.setTargetValue(clamped);
    }

    void Bitcrush::setTargetSampleRate(float hz)
    {
        const float clamped = std::max(20.0f, hz);
        if (firstSetAfterPrepare)
            targetHzSmoother.setCurrentAndTargetValue(clamped);
        else
            targetHzSmoother.setTargetValue(clamped);
    }

    float Bitcrush::processSample(float input)
    {
        if (! prepared)
            return 0.0f;

        // Advance the smoothers per sample and recompute the derived
        // quantization step + phase increment. Both ops are cheap
        // (pow + divide), so per-sample is fine — no throttle needed.
        bitDepth = bitDepthSmoother.getNextValue();
        targetHz = targetHzSmoother.getNextValue();
        recomputeQuantStep();
        recomputePhaseIncrement();
        firstSetAfterPrepare = false;

        if (phase >= 1.0)
        {
            // floor handles targetHz > sampleRate (phase increments
            // larger than 1) — phase always ends up in [0, 1).
            phase     -= std::floor(phase);
            heldSample = quantize(input);
        }
        phase += phaseIncrement;
        return heldSample;
    }

    float Bitcrush::quantize(float x) const
    {
        return std::round(x / quantStep) * quantStep;
    }

    void Bitcrush::recomputeQuantStep()
    {
        quantStep = 2.0f / std::pow(2.0f, bitDepth);
    }

    void Bitcrush::recomputePhaseIncrement()
    {
        if (sampleRate > 0.0)
            phaseIncrement = static_cast<double>(targetHz) / sampleRate;
    }
}

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
    }

    void Bitcrush::reset()
    {
        phase      = 1.0;
        heldSample = 0.0f;
    }

    void Bitcrush::setBitDepth(float bits)
    {
        bitDepth = std::clamp(bits, 1.0f, 16.0f);
        recomputeQuantStep();
    }

    void Bitcrush::setTargetSampleRate(float hz)
    {
        targetHz = std::max(20.0f, hz);
        recomputePhaseIncrement();
    }

    float Bitcrush::processSample(float input)
    {
        if (! prepared)
            return 0.0f;

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

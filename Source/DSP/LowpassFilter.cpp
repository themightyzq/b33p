#include "LowpassFilter.h"

#include <algorithm>

namespace B33p
{
    void LowpassFilter::prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        prepared   = true;
        filter.reset();
        updateCoefficients();
    }

    void LowpassFilter::reset()
    {
        filter.reset();
    }

    void LowpassFilter::setCutoff(float hz)
    {
        cutoffHz = hz;
        updateCoefficients();
    }

    void LowpassFilter::setResonance(float q)
    {
        resonanceQ = q;
        updateCoefficients();
    }

    float LowpassFilter::processSample(float input)
    {
        if (! prepared)
            return 0.0f;

        return filter.processSample(input);
    }

    void LowpassFilter::updateCoefficients()
    {
        if (! prepared)
            return;

        // 0.499 * sampleRate keeps the cutoff strictly below Nyquist
        // with a hair of margin; biquad math is well-behaved there.
        const float nyquistLimit = static_cast<float>(sampleRate * 0.499);
        const float safeCutoff   = std::clamp(cutoffHz,   20.0f, nyquistLimit);
        const float safeQ        = std::clamp(resonanceQ, 0.1f,  20.0f);

        filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sampleRate, safeCutoff, safeQ);
    }
}

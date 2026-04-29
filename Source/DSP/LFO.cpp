#include "LFO.h"

#include <algorithm>
#include <cmath>

namespace B33p
{
    namespace
    {
        constexpr double kTwoPi = 6.283185307179586;

        float evaluateShape(LFO::Shape shape, double phase) noexcept
        {
            switch (shape)
            {
                case LFO::Shape::Sine:
                    return static_cast<float>(std::sin(kTwoPi * phase));
                case LFO::Shape::Triangle:
                    if (phase < 0.25)
                        return static_cast<float>(4.0 * phase);
                    if (phase < 0.75)
                        return static_cast<float>(2.0 - 4.0 * phase);
                    return static_cast<float>(4.0 * phase - 4.0);
                case LFO::Shape::Saw:
                    return static_cast<float>(2.0 * phase - 1.0);
                case LFO::Shape::Square:
                    return phase < 0.5 ? 1.0f : -1.0f;
            }
            return 0.0f;
        }
    }

    void LFO::prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        prepared   = true;
    }

    void LFO::reset()
    {
        phase = 0.0;
    }

    void LFO::setShape(Shape newShape)
    {
        shape = newShape;
    }

    void LFO::setRate(float hz)
    {
        // 0..30 Hz covers everything from a multi-second wobble up
        // through audible-territory tremolo. Anything above is
        // perceptually identical to the carrier signal.
        rateHz = std::clamp(hz, 0.0f, 30.0f);
    }

    float LFO::currentValue() const
    {
        if (! prepared)
            return 0.0f;
        return evaluateShape(shape, phase);
    }

    void LFO::advance(int numSamples)
    {
        if (! prepared || rateHz <= 0.0f || numSamples <= 0)
            return;
        const double increment = static_cast<double>(rateHz)
                                  * static_cast<double>(numSamples)
                                  / sampleRate;
        phase += increment;
        // Phase stays in [0, 1) so cumulative drift across long
        // sessions doesn't cost precision.
        phase -= std::floor(phase);
    }
}

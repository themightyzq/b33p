#include "Oscillator.h"

#include <cmath>

namespace B33p
{
    namespace
    {
        constexpr double kTwoPi = 6.283185307179586;

        float sineAt(double phase) noexcept
        {
            return static_cast<float>(std::sin(kTwoPi * phase));
        }

        float squareAt(double phase) noexcept
        {
            return phase < 0.5 ? 1.0f : -1.0f;
        }

        float sawAt(double phase) noexcept
        {
            return static_cast<float>(2.0 * phase - 1.0);
        }

        // Triangle shares sine's zero crossings at phase 0 and 0.5 so the
        // five waveforms swap cleanly at runtime without phase discontinuity.
        float triangleAt(double phase) noexcept
        {
            if (phase < 0.25)
                return static_cast<float>(4.0 * phase);
            if (phase < 0.75)
                return static_cast<float>(2.0 - 4.0 * phase);
            return static_cast<float>(4.0 * phase - 4.0);
        }
    }

    Oscillator::Oscillator()
        : rng(std::random_device{}())
    {
        updatePhaseIncrement();
    }

    void Oscillator::prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        updatePhaseIncrement();
    }

    void Oscillator::reset()
    {
        phase = 0.0;
    }

    void Oscillator::setWaveform(Waveform newWaveform)
    {
        waveform = newWaveform;
    }

    void Oscillator::setFrequency(float hz)
    {
        frequencyHz = hz;
        updatePhaseIncrement();
    }

    void Oscillator::updatePhaseIncrement()
    {
        phaseIncrement = sampleRate > 0.0
            ? static_cast<double>(frequencyHz) / sampleRate
            : 0.0;
    }

    float Oscillator::processSample()
    {
        if (sampleRate <= 0.0)
            return 0.0f;

        if (waveform == Waveform::Noise)
            return noiseDist(rng);

        float output = 0.0f;
        switch (waveform)
        {
            case Waveform::Sine:     output = sineAt(phase);     break;
            case Waveform::Square:   output = squareAt(phase);   break;
            case Waveform::Triangle: output = triangleAt(phase); break;
            case Waveform::Saw:      output = sawAt(phase);      break;
            case Waveform::Noise:    break; // handled above
        }

        phase += phaseIncrement;
        while (phase >= 1.0)
            phase -= 1.0;

        return output;
    }
}

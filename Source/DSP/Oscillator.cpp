#include "Oscillator.h"

#include <algorithm>
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

        // Linear-interpolated lookup of a custom single-cycle table.
        // Empty table = silence so an unconfigured Custom waveform
        // doesn't blast random memory or click.
        float customAt(double phase, const std::vector<float>& t) noexcept
        {
            if (t.empty())
                return 0.0f;
            const double scaled = phase * static_cast<double>(t.size());
            const std::size_t i  = static_cast<std::size_t>(scaled) % t.size();
            const std::size_t j  = (i + 1) % t.size();
            const float frac     = static_cast<float>(scaled - std::floor(scaled));
            return t[i] + frac * (t[j] - t[i]);
        }

        // Linear blend between two adjacent wavetable slots.
        // morph01 is normalised to a [0, slotCount-1] position
        // before being split into integer + fractional parts.
        float wavetableAt(double phase,
                           const std::array<std::vector<float>,
                                            Oscillator::kNumWavetableSlots>& slots,
                           float morph01) noexcept
        {
            constexpr int last = Oscillator::kNumWavetableSlots - 1;
            const float clamped = std::clamp(morph01, 0.0f, 1.0f);
            const float pos     = clamped * static_cast<float>(last);
            const int   lo      = std::clamp(static_cast<int>(std::floor(pos)), 0, last);
            const int   hi      = std::min(lo + 1, last);
            const float frac    = pos - static_cast<float>(lo);

            const float a = customAt(phase, slots[static_cast<std::size_t>(lo)]);
            const float b = customAt(phase, slots[static_cast<std::size_t>(hi)]);
            return a + frac * (b - a);
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

    void Oscillator::setCustomTable(const std::vector<float>& samples)
    {
        setWavetableSlot(0, samples);
    }

    void Oscillator::setWavetableSlot(int slot, const std::vector<float>& samples)
    {
        if (slot < 0 || slot >= kNumWavetableSlots)
            return;
        wavetableSlots[static_cast<std::size_t>(slot)] = samples;
    }

    void Oscillator::setWavetableMorph(float morph01)
    {
        wavetableMorph = std::clamp(morph01, 0.0f, 1.0f);
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
            case Waveform::Sine:      output = sineAt(phase);                                         break;
            case Waveform::Square:    output = squareAt(phase);                                       break;
            case Waveform::Triangle:  output = triangleAt(phase);                                     break;
            case Waveform::Saw:       output = sawAt(phase);                                          break;
            case Waveform::Custom:    output = customAt(phase, wavetableSlots[0]);                    break;
            case Waveform::Wavetable: output = wavetableAt(phase, wavetableSlots, wavetableMorph);    break;
            case Waveform::Noise:     break; // handled above
        }

        phase += phaseIncrement;
        while (phase >= 1.0)
            phase -= 1.0;

        return output;
    }
}

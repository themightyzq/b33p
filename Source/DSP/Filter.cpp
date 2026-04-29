#include "Filter.h"

#include <algorithm>
#include <cmath>

namespace B33p
{
    namespace
    {
        // Five vowels' first three formant frequencies in Hz. Values
        // are the standard reference set used in textbook formant
        // synthesis examples; they're not committed to a specific
        // language or speaker. F1, F2, F3 in that column order.
        constexpr int kNumFormants = 3;
        constexpr float kVowelFormants[Filter::kNumVowels][kNumFormants] = {
            { 700.0f, 1200.0f, 2500.0f },  // A
            { 400.0f, 2000.0f, 2700.0f },  // E
            { 270.0f, 2300.0f, 3000.0f },  // I
            { 400.0f,  800.0f, 2400.0f },  // O
            { 300.0f,  600.0f, 2300.0f },  // U
        };
        constexpr float kFormantQ = 12.0f;

        // Lowest comb fundamental we ever expect to support. Sets
        // the delay-line size in prepare(). Below ~50 Hz the comb
        // teeth are too dense to hear individually anyway.
        constexpr float kMinCombHz = 50.0f;

        // Round up to next power of two — keeps the modulo-by-mask
        // index advance branch-free.
        std::size_t nextPow2(std::size_t v)
        {
            std::size_t p = 1;
            while (p < v) p <<= 1;
            return p;
        }

        // Linearly interpolate a vowel's three formant frequencies
        // between two adjacent vowels in the table. vowel01 is
        // 0 = A, 1 = U with the intermediate vowels evenly spaced.
        std::array<float, kNumFormants> resolveVowelFormants(float vowel01)
        {
            const float clamped = std::clamp(vowel01, 0.0f, 1.0f);
            const float pos     = clamped * static_cast<float>(Filter::kNumVowels - 1);
            const int   lo      = std::clamp(static_cast<int>(std::floor(pos)),
                                             0, Filter::kNumVowels - 1);
            const int   hi      = std::min(lo + 1, Filter::kNumVowels - 1);
            const float frac    = pos - static_cast<float>(lo);

            std::array<float, kNumFormants> out;
            for (int i = 0; i < kNumFormants; ++i)
                out[static_cast<size_t>(i)] =
                    kVowelFormants[lo][i] + frac * (kVowelFormants[hi][i] - kVowelFormants[lo][i]);
            return out;
        }
    }

    void Filter::prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        prepared   = true;
        biquad.reset();
        for (auto& b : formantBands)
            b.reset();

        // Worst-case delay in samples for the lowest comb fundamental
        // we support. Round up to the next power of two so the
        // wrap-around on combWriteIx is a cheap AND with combMask.
        const std::size_t worstDelay = static_cast<std::size_t>(
            std::ceil(sampleRate / static_cast<double>(kMinCombHz)));
        const std::size_t pow2 = std::max<std::size_t>(nextPow2(worstDelay + 4), 16);
        combBuffer.assign(pow2, 0.0f);
        combMask    = pow2 - 1;
        combWriteIx = 0;

        updateCoefficients();
    }

    void Filter::reset()
    {
        biquad.reset();
        for (auto& b : formantBands)
            b.reset();
        std::fill(combBuffer.begin(), combBuffer.end(), 0.0f);
        combWriteIx = 0;
    }

    void Filter::setType(Type newType)
    {
        if (type == newType)
            return;
        type = newType;
        updateCoefficients();
    }

    void Filter::setCutoff(float hz)
    {
        cutoffHz = hz;
        updateCoefficients();
    }

    void Filter::setResonance(float q)
    {
        resonanceQ = q;
        updateCoefficients();
    }

    void Filter::setVowel(float v01)
    {
        vowel01 = std::clamp(v01, 0.0f, 1.0f);
        updateCoefficients();
    }

    float Filter::combFeedbackFromQ() const
    {
        // Map the Q range used by the resonance slider [0.1, 20]
        // onto a feedback gain in [0, 0.95]. 0.95 is the highest
        // stable feedback for a feedback-comb without runaway.
        const float t = std::clamp((resonanceQ - 0.1f) / (20.0f - 0.1f), 0.0f, 1.0f);
        return t * 0.95f;
    }

    std::size_t Filter::combDelaySamples() const
    {
        // Delay = sample rate / cutoffHz. Clamp to at least 1 to
        // avoid zero-length delay (degenerate identity filter).
        const double s = sampleRate / static_cast<double>(std::max(cutoffHz, kMinCombHz));
        return std::max<std::size_t>(static_cast<std::size_t>(s), 1u);
    }

    void Filter::updateCoefficients()
    {
        if (! prepared)
            return;

        const float nyquistLimit = static_cast<float>(sampleRate * 0.499);
        const float safeCutoff   = std::clamp(cutoffHz,   20.0f, nyquistLimit);
        const float safeQ        = std::clamp(resonanceQ, 0.1f,  20.0f);

        switch (type)
        {
            case Type::Lowpass:
                biquad.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(
                    sampleRate, safeCutoff, safeQ);
                break;
            case Type::Highpass:
                biquad.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(
                    sampleRate, safeCutoff, safeQ);
                break;
            case Type::Bandpass:
                biquad.coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(
                    sampleRate, safeCutoff, safeQ);
                break;
            case Type::Comb:
                // No biquad coefficients to install — the comb mode
                // reads directly from combBuffer in processSample.
                break;
            case Type::Formant:
            {
                const auto formants = resolveVowelFormants(vowel01);
                for (int i = 0; i < kNumFormants; ++i)
                {
                    const float f = std::clamp(formants[static_cast<size_t>(i)],
                                                20.0f, nyquistLimit);
                    formantBands[static_cast<size_t>(i)].coefficients =
                        juce::dsp::IIR::Coefficients<float>::makeBandPass(
                            sampleRate, f, kFormantQ);
                }
                break;
            }
        }
    }

    float Filter::processSample(float input)
    {
        if (! prepared)
            return 0.0f;

        switch (type)
        {
            case Type::Lowpass:
            case Type::Highpass:
            case Type::Bandpass:
                return biquad.processSample(input);

            case Type::Comb:
            {
                // Feedback comb: y[n] = x[n] + g * y[n-D].
                // Read tap is D samples behind the current write
                // position; mask handles the wrap-around because
                // combBuffer is sized to a power of two.
                const std::size_t delay   = combDelaySamples();
                const std::size_t readIx  = (combWriteIx + combBuffer.size() - delay) & combMask;
                const float       fed     = combBuffer[readIx];
                const float       output  = input + combFeedbackFromQ() * fed;
                combBuffer[combWriteIx] = output;
                combWriteIx = (combWriteIx + 1) & combMask;
                return output;
            }

            case Type::Formant:
            {
                // Sum the three formant bandpasses. Divide by the
                // band count so a flat-response input doesn't push
                // the output beyond the carrier's peak amplitude.
                float sum = 0.0f;
                for (auto& b : formantBands)
                    sum += b.processSample(input);
                return sum / static_cast<float>(formantBands.size());
            }
        }
        return 0.0f;
    }
}

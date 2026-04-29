#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <vector>

namespace B33p
{
    // Multi-mode filter. Owns a single biquad for the simple
    // analog-modelled types (LP/HP/BP), a feedback comb delay line
    // for Comb mode, and three parallel bandpass biquads tuned to
    // formant frequencies for Formant mode.
    //
    // Parameters are reused across modes, with type-dependent
    // semantics so the existing two-knob UI keeps working:
    //   LP/HP/BP : cutoff = corner / centre frequency, Q = resonance
    //   Comb     : cutoff = comb peak fundamental (delay = 1/cutoff),
    //              Q     -> feedback gain (mapped from [0.1, 20] to
    //              the linear range [0, 0.95]) — higher Q = more
    //              prominent comb teeth
    //   Formant  : cutoff and Q are unused; the dedicated vowel
    //              parameter (0..1) interpolates across the five
    //              vowel formant tables A, E, I, O, U
    //
    // Lifecycle: construct -> prepare(sampleRate) -> setType /
    // setCutoff / setResonance / setVowel in any order ->
    // processSample. Before prepare(), processSample returns silence
    // (0.0f). reset() returns the filter to a fresh runtime state
    // (delay-line memory cleared, biquad memory zeroed).
    class Filter
    {
    public:
        enum class Type
        {
            Lowpass,
            Highpass,
            Bandpass,
            Comb,
            Formant
        };

        void prepare(double sampleRate);
        void reset();

        void setType(Type type);
        void setCutoff(float hz);
        void setResonance(float q);
        void setVowel(float vowel01);

        float processSample(float input);

        // Number of vowels the formant lookup covers. Public so the
        // visualizer / UI can iterate over them or label vowel
        // positions without duplicating the count.
        static constexpr int kNumVowels = 5;

    private:
        void   updateCoefficients();
        float  combFeedbackFromQ() const;
        size_t combDelaySamples() const;

        Type   type       { Type::Lowpass };
        double sampleRate { 0.0 };
        float  cutoffHz   { 20000.0f };
        float  resonanceQ { 0.707f };
        float  vowel01    { 0.0f };
        bool   prepared   { false };

        // Single biquad used by LP/HP/BP. For Comb/Formant it is
        // reset and bypassed.
        juce::dsp::IIR::Filter<float> biquad;

        // Comb feedback delay line. Sized in prepare() to comfortably
        // hold the longest delay we'd ever need (sampleRate / lowest
        // useful comb fundamental); we round up to the next power of
        // two to keep index masking cheap. The actual delay used per
        // sample comes from cutoffHz at runtime.
        std::vector<float> combBuffer;
        std::size_t        combMask    { 0 };
        std::size_t        combWriteIx { 0 };

        // Three parallel bandpass biquads for Formant mode, each
        // tuned to one of the vowel's formant frequencies (F1..F3).
        std::array<juce::dsp::IIR::Filter<float>, 3> formantBands;
    };
}

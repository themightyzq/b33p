#pragma once

#include <random>
#include <vector>

namespace B33p
{
    // Monophonic naive (non-band-limited) oscillator. Short, pitch-swept beeps
    // live comfortably below audible aliasing on most of this range, so we
    // skip band-limiting for MVP. Anti-aliasing is a post-MVP concern.
    //
    // Lifecycle: construct -> prepare(sampleRate) -> setWaveform/setFrequency
    // -> processSample() per output sample. Before prepare() is called,
    // processSample() returns silence (0.0f).
    class Oscillator
    {
    public:
        enum class Waveform
        {
            Sine,
            Square,
            Triangle,
            Saw,
            Noise,
            Custom
        };

        // Default custom-table size — small enough that the full
        // table fits in CPU cache, large enough that a freely-drawn
        // shape doesn't sound aliased at typical pitches.
        static constexpr int kCustomTableSize = 256;

        Oscillator();

        void prepare(double sampleRate);
        void reset();

        void setWaveform(Waveform waveform);
        void setFrequency(float hz);

        // Replaces the per-cycle sample table used when waveform is
        // Custom. Empty / wrong-size tables fall back to silence.
        // Linear-interpolated lookup at the current phase position.
        void setCustomTable(const std::vector<float>& samples);

        float processSample();

    private:
        void updatePhaseIncrement();

        double   sampleRate     { 0.0 };
        float    frequencyHz    { 440.0f };
        double   phase          { 0.0 };
        double   phaseIncrement { 0.0 };
        Waveform waveform       { Waveform::Sine };

        std::vector<float>                    customTable;

        std::mt19937                          rng;
        std::uniform_real_distribution<float> noiseDist { -1.0f, 1.0f };
    };
}

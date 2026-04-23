#pragma once

#include <random>

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
            Noise
        };

        Oscillator();

        void prepare(double sampleRate);
        void reset();

        void setWaveform(Waveform waveform);
        void setFrequency(float hz);

        float processSample();

    private:
        void updatePhaseIncrement();

        double   sampleRate     { 0.0 };
        float    frequencyHz    { 440.0f };
        double   phase          { 0.0 };
        double   phaseIncrement { 0.0 };
        Waveform waveform       { Waveform::Sine };

        std::mt19937                          rng;
        std::uniform_real_distribution<float> noiseDist { -1.0f, 1.0f };
    };
}

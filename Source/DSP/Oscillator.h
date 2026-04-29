#pragma once

#include <array>
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
    //
    // Wavetable mode: linearly interpolates between four single-cycle
    // tables (slot 0..3) based on a 0..1 morph value, where 0 = pure
    // slot 0, 1 = pure slot 3, and intermediate values blend two
    // adjacent slots. Custom mode reuses slot 0 — the difference is
    // whether the morph parameter participates.
    //
    // FM mode: classic two-operator sine FM. The carrier runs at
    // basePitchHz and is phase-modulated by a sine modulator running
    // at basePitchHz * fmRatio with depth fmDepth (modulation index).
    // Both operators are sines; future modes can extend the palette.
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
            Custom,
            Wavetable,
            FM
        };

        // Default custom-table size — small enough that the full
        // table fits in CPU cache, large enough that a freely-drawn
        // shape doesn't sound aliased at typical pitches.
        static constexpr int kCustomTableSize  = 256;

        // Wavetable slot count. Fixed at four for MVP. The morph
        // parameter spans [0, kNumWavetableSlots - 1] internally; the
        // public API normalises that to a 0..1 input.
        static constexpr int kNumWavetableSlots = 4;

        Oscillator();

        void prepare(double sampleRate);
        void reset();

        void setWaveform(Waveform waveform);
        void setFrequency(float hz);

        // Replaces the per-cycle sample table used when waveform is
        // Custom. Convenience alias for setWavetableSlot(0, samples)
        // — Custom mode reads slot 0 of the wavetable storage.
        void setCustomTable(const std::vector<float>& samples);

        // Replaces a single wavetable slot. slot is clamped to
        // [0, kNumWavetableSlots). Empty / wrong-size tables play
        // silence for that slot.
        void setWavetableSlot(int slot, const std::vector<float>& samples);

        // 0..1 morph position across the kNumWavetableSlots slots.
        // 0 = pure slot 0, 1 = pure slot N-1, intermediate values
        // linearly blend the two adjacent slots. Out-of-range inputs
        // are clamped.
        void setWavetableMorph(float morph01);

        // FM-mode parameters. ratio = modulator frequency divided by
        // carrier frequency (1.0 = same pitch, 2.0 = octave-up
        // modulator, etc.). depth = modulation index, in radians of
        // carrier-phase deviation per modulator-output unit. 0 turns
        // the modulator off (pure sine carrier). Both inputs are
        // clamped to sane ranges.
        void setFmRatio(float ratio);
        void setFmDepth(float depth);

        float processSample();

    private:
        void updatePhaseIncrement();

        double   sampleRate     { 0.0 };
        float    frequencyHz    { 440.0f };
        double   phase          { 0.0 };
        double   phaseIncrement { 0.0 };
        Waveform waveform       { Waveform::Sine };

        std::array<std::vector<float>, kNumWavetableSlots> wavetableSlots;
        float                                              wavetableMorph { 0.0f };

        // FM modulator runs in parallel with the carrier phase. Kept
        // as double for the same drift-free reasons as the main
        // accumulator; reset() zeros it alongside the carrier phase.
        double modulatorPhase { 0.0 };
        float  fmRatio        { 1.0f };
        float  fmDepth        { 0.0f };

        std::mt19937                          rng;
        std::uniform_real_distribution<float> noiseDist { -1.0f, 1.0f };
    };
}

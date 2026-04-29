#pragma once

namespace B33p
{
    // Low-frequency oscillator for the modulation engine. Produces a
    // bipolar (-1..+1) output in one of four shapes. Phase is
    // advanced explicitly by the host (B33pProcessor advances by
    // numSamples once per audio block and reads currentValue() for
    // its block-rate modulation matrix).
    //
    // LFOs run free — they do not retrigger at note events, so a
    // sustained pad can ride a slow filter sweep across many notes
    // without the LFO snapping back to phase 0 on each trigger.
    //
    // Lifecycle: construct -> prepare(sampleRate) -> setShape /
    // setRate in any order -> currentValue / advance loop. Before
    // prepare(), currentValue() returns 0.
    class LFO
    {
    public:
        enum class Shape { Sine, Triangle, Saw, Square };

        void prepare(double sampleRate);
        void reset();

        void setShape(Shape shape);
        void setRate(float hz);

        // Reads the LFO's current output without advancing the phase.
        // Returns 0 when the LFO hasn't been prepare()'d yet.
        float currentValue() const;

        // Advances the phase accumulator by numSamples worth of LFO
        // rotation. Called once per audio block by the host.
        void  advance(int numSamples);

    private:
        Shape  shape       { Shape::Sine };
        float  rateHz      { 1.0f };
        double sampleRate  { 0.0 };
        double phase       { 0.0 };
        bool   prepared    { false };
    };
}

#pragma once

namespace B33p
{
    // Memoryless tanh waveshaper: processSample(x) = tanh(drive * x).
    //
    // Drive is a pre-gain multiplier clamped to [0.1, 100]. At drive=1
    // the output closely tracks the input for small-amplitude signals
    // and softly saturates at peaks; higher drive pushes tanh toward
    // its ±1 asymptote, producing progressively more harmonic content.
    // Output is always bounded in (-1, 1).
    //
    // There is no internal state — reset() is a documented no-op, kept
    // for lifecycle consistency with the other DSP primitives. prepare()
    // only flips the prepared flag; sample rate does not affect the
    // transfer function (no oversampling in MVP). Before prepare(),
    // processSample() returns silence (0.0f).
    class Distortion
    {
    public:
        void prepare(double sampleRate);
        void reset();

        void setDrive(float drive);

        float processSample(float input);

    private:
        float drive    { 1.0f };
        bool  prepared { false };
    };
}

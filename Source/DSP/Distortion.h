#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

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
    // transfer function. Before prepare(), processSample() returns
    // silence (0.0f).
    //
    // This class itself has no oversampling and no latency -- it stays
    // a plain memoryless waveshaper so its own tests can assert exact,
    // immediate transfer-function values. OversampledDistortion.h wraps
    // an instance of this class with 4x oversampling for the anti-alias
    // benefit; Voice uses the wrapper, not this class directly.
    class Distortion
    {
    public:
        void prepare(double sampleRate);
        void reset();

        void setDrive(float drive);

        float processSample(float input);

        // Current (smoothed) drive value as of the most recent
        // processSample() call. Exists so OversampledDistortion
        // (OversampledDistortion.h) can advance the drive smoother once
        // per host-rate sample and re-apply that same value across every
        // sample of a 4x-oversampled block via shape() below, instead of
        // advancing the smoother once per oversampled tick (which would
        // finish the 20 ms ramp 4x faster than CLAUDE.md's smoothing
        // budget intends).
        float getCurrentDrive() const { return drive; }

        // The waveshaper's transfer function in isolation, with no
        // smoothing and no state -- tanh(drive * input). processSample()
        // is built on this; OversampledDistortion also calls it directly
        // to shape the oversampled ticks it doesn't run processSample()
        // for (see getCurrentDrive() above).
        static float shape(float input, float drive) noexcept;

    private:
        float                      drive         { 1.0f };
        bool                       prepared      { false };
        // Drive smoother — 20 ms ramp. Fast drive automation otherwise
        // zippers the tanh waveshape (CLAUDE.md "Parameter smoothing").
        juce::SmoothedValue<float> driveSmoother;
        bool                       firstSetAfterPrepare { true };
    };
}

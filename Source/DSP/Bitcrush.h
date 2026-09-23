#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace B33p
{
    // Sample-and-hold sample-rate reduction combined with uniform
    // bit-depth quantization. Two knobs, no drive, no dithering.
    //
    // Bit depth is a float in [1, 16]; non-integer values produce
    // intermediate crunch (step size = 2 / 2^bits). Mid-tread grid —
    // zero is a valid output level, so 1-bit yields {-1, 0, +1}.
    //
    // Target sample rate is held >= 20 Hz. A target above the host
    // sample rate is a no-op for the rate-reduction stage (every
    // input sample captures).
    //
    // The phase accumulator starts at 1.0 so the first call to
    // processSample() after prepare() / reset() always captures the
    // incoming input — avoids a one-sample silent hole.
    //
    // Lifecycle: construct -> prepare(sampleRate) -> setBitDepth /
    // setTargetSampleRate -> processSample ... . Before prepare(),
    // processSample() returns silence (0.0f).
    //
    // This class itself has no oversampling and no latency -- it stays
    // a plain sample-and-hold quantizer so its own tests can assert
    // exact, immediate sample values. OversampledBitcrush.h wraps an
    // instance of this class with 4x oversampling for the anti-alias
    // benefit; Voice uses the wrapper, not this class directly.
    class Bitcrush
    {
    public:
        void prepare(double sampleRate);
        void reset();

        void setBitDepth(float bits);
        void setTargetSampleRate(float targetHz);

        float processSample(float input);

        // --- Oversampling support (OversampledBitcrush.h only) --------
        //
        // processSample() above fuses three things every call: advancing
        // the bit-depth/rate smoothers, recomputing the derived
        // quantStep/phaseIncrement, and running one sample-and-hold tick.
        // OversampledBitcrush needs to split that: advance the smoothers
        // once per host-rate sample (so their 30 ms ramps keep the timing
        // CLAUDE.md's "Parameter smoothing" section specifies), but run
        // the sample-and-hold tick once per oversampled sub-sample, with
        // the phase increment computed against the oversampled tick rate
        // rather than the host sample rate -- otherwise the "hold"
        // captures targetHz*factor times a second instead of targetHz,
        // changing the audible reduction ratio the factor is supposed to
        // leave alone.
        //
        // beginOversampledTick(tickRateHz) is the once-per-host-sample
        // half (smoothers + quantStep + phase increment, using
        // tickRateHz in place of sampleRate); step() is the per-tick half
        // (the sample-and-hold + quantize itself, using whatever phase
        // increment is currently set). processSample() is unchanged and
        // still does both at the host rate in one call, for callers that
        // don't oversample (i.e. its own unit tests).
        void  beginOversampledTick(double tickRateHz);
        float step(float input);

    private:
        float quantize(float x) const;
        void  recomputeQuantStep();
        void  recomputePhaseIncrement();

        double sampleRate     { 0.0 };
        float  bitDepth       { 16.0f };
        float  targetHz       { 48000.0f };

        float  quantStep      { 2.0f / 65536.0f };
        double phaseIncrement { 1.0 };

        double phase          { 1.0 };
        float  heldSample     { 0.0f };
        bool   prepared       { false };

        // Per-sample smoothers — fast automation otherwise zippers both
        // params (quantStep jumps at bitDepth edges, phaseIncrement
        // jumps at targetHz edges). CLAUDE.md "Parameter smoothing".
        juce::SmoothedValue<float> bitDepthSmoother;
        juce::SmoothedValue<float> targetHzSmoother;
        bool                       firstSetAfterPrepare { true };
    };
}

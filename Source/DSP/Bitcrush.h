#pragma once

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
    class Bitcrush
    {
    public:
        void prepare(double sampleRate);
        void reset();

        void setBitDepth(float bits);
        void setTargetSampleRate(float targetHz);

        float processSample(float input);

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
    };
}

#pragma once

namespace B33p
{
    // Memoryless soft-clip safety limiter on the final mixed output.
    //
    // The four lane voices plus the MIDI voice pool sum with no headroom,
    // so a dense pattern (or a hot randomized patch) can push the sum well
    // past +/-1.0 and hard-clip. This stage prevents that: below the knee
    // threshold the transfer is identity, so clean signal passes untouched;
    // above it an exponential soft-knee rounds peaks, asymptotically
    // approaching a ceiling set just below 0 dBFS (for huge inputs the
    // exponential underflows, so the output bottoms out at exactly the
    // ceiling). The result is always bounded at or below that ceiling, so
    // the summed output can't reach full-scale, can't hard-clip, and the
    // meter stays out of the red. The curve is C1-continuous at the knee
    // (unit slope on both sides), so there's no audible kink where
    // limiting begins.
    //
    // Stateless: reset() is a documented no-op and prepare() only flips the
    // prepared flag (the transfer function is sample-rate independent — no
    // oversampling). Before prepare(), processSample() returns silence
    // (0.0f), matching the other DSP primitives' lifecycle contract.
    class OutputLimiter
    {
    public:
        void prepare(double sampleRate);
        void reset();

        float processSample(float input);

        // Linear amplitudes. Identity transfer holds for |x| <= kKnee;
        // the soft-knee rounds |x| > kKnee toward `kCeiling`. Exposed so
        // tests can assert the boundary behaviour against the same values.
        static constexpr float kKnee    = 0.70f;    // ~ -3.1 dBFS
        static constexpr float kCeiling = 0.966f;   // ~ -0.3 dBFS

    private:
        bool prepared { false };
    };
}

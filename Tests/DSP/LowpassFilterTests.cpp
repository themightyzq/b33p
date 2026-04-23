#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/LowpassFilter.h"

#include <cmath>
#include <random>

using B33p::LowpassFilter;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr double kTwoPi      = 6.28318530717958647692;

    void warmupSine(LowpassFilter& lpf, double frequencyHz, int numSamples)
    {
        double phase = 0.0;
        const double phaseIncrement = kTwoPi * frequencyHz / kSampleRate;
        for (int i = 0; i < numSamples; ++i)
        {
            lpf.processSample(static_cast<float>(std::sin(phase)));
            phase += phaseIncrement;
        }
    }

    double rmsOfSineOutput(LowpassFilter& lpf, double frequencyHz, int numSamples)
    {
        double phase = 0.0;
        const double phaseIncrement = kTwoPi * frequencyHz / kSampleRate;
        double sumSq = 0.0;
        for (int i = 0; i < numSamples; ++i)
        {
            const float x = static_cast<float>(std::sin(phase));
            phase += phaseIncrement;
            const float y = lpf.processSample(x);
            sumSq += static_cast<double>(y) * static_cast<double>(y);
        }
        return std::sqrt(sumSq / static_cast<double>(numSamples));
    }
}

TEST_CASE("LowpassFilter: processSample without prepare returns silence", "[dsp][lpf]")
{
    LowpassFilter lpf;
    lpf.setCutoff(1000.0f);
    lpf.setResonance(1.0f);

    for (int i = 0; i < 64; ++i)
        REQUIRE(lpf.processSample(1.0f) == 0.0f);
}

TEST_CASE("LowpassFilter: reset clears filter memory", "[dsp][lpf]")
{
    LowpassFilter lpf;
    lpf.prepare(kSampleRate);
    lpf.setCutoff(1000.0f);
    lpf.setResonance(0.707f);

    // Pump DC until the delay line is fully charged.
    for (int i = 0; i < 10000; ++i)
        (void) lpf.processSample(1.0f);

    lpf.reset();

    // Zero state + zero input must produce zero output.
    for (int i = 0; i < 200; ++i)
        REQUIRE(lpf.processSample(0.0f) == Approx(0.0f).margin(1e-6));
}

TEST_CASE("LowpassFilter: DC input passes through at unity gain", "[dsp][lpf]")
{
    LowpassFilter lpf;
    lpf.prepare(kSampleRate);
    lpf.setCutoff(1000.0f);
    lpf.setResonance(0.707f);

    float out = 0.0f;
    for (int i = 0; i < 10000; ++i)
        out = lpf.processSample(1.0f);

    // Biquad lowpass DC gain is exactly 1.0 by construction.
    REQUIRE(out == Approx(1.0f).margin(1e-3f));
}

TEST_CASE("LowpassFilter: in-passband sine passes with near-unity gain", "[dsp][lpf]")
{
    LowpassFilter lpf;
    lpf.prepare(kSampleRate);
    lpf.setCutoff(8000.0f);
    lpf.setResonance(0.707f);

    warmupSine(lpf, 400.0, 10000);

    const double rmsIn  = std::sqrt(0.5);  // analytic RMS of a unit-amplitude sine
    const double rmsOut = rmsOfSineOutput(lpf, 400.0, 48000);
    const double gainDb = 20.0 * std::log10(rmsOut / rmsIn);

    // 400 Hz is 4.3 octaves below cutoff — essentially flat.
    REQUIRE(gainDb > -0.5);
    REQUIRE(gainDb <  0.5);
}

TEST_CASE("LowpassFilter: far-stopband sine is strongly attenuated", "[dsp][lpf]")
{
    LowpassFilter lpf;
    lpf.prepare(kSampleRate);
    lpf.setCutoff(500.0f);
    lpf.setResonance(0.707f);

    warmupSine(lpf, 10000.0, 10000);

    const double rmsIn  = std::sqrt(0.5);
    const double rmsOut = rmsOfSineOutput(lpf, 10000.0, 48000);
    const double gainDb = 20.0 * std::log10(rmsOut / rmsIn);

    // 10 kHz is 20x above cutoff (~4.3 octaves). A 12 dB/oct biquad
    // gives ~52 dB of attenuation there; require at least 30 dB.
    REQUIRE(gainDb < -30.0);
}

TEST_CASE("LowpassFilter: higher resonance Q amplifies at cutoff frequency", "[dsp][lpf]")
{
    auto rmsAtCutoff = [](float q)
    {
        LowpassFilter lpf;
        lpf.prepare(kSampleRate);
        lpf.setCutoff(1000.0f);
        lpf.setResonance(q);

        warmupSine(lpf, 1000.0, 20000);
        return rmsOfSineOutput(lpf, 1000.0, 48000);
    };

    const double rmsLowQ  = rmsAtCutoff(0.707f);  // biquad gain at cutoff = Q
    const double rmsHighQ = rmsAtCutoff(5.0f);

    // Expected ratio ~5 / 0.707 ~= 7. Require at least 4x to leave
    // margin for warmup transients and floating-point tolerance.
    REQUIRE(rmsHighQ > rmsLowQ * 4.0);
}

TEST_CASE("LowpassFilter: cutoff well above Nyquist stays stable", "[dsp][lpf]")
{
    LowpassFilter lpf;
    lpf.prepare(kSampleRate);
    lpf.setResonance(0.707f);
    lpf.setCutoff(100000.0f);  // clamps to ~0.499 * sampleRate

    for (int i = 0; i < 10000; ++i)
    {
        const float y = lpf.processSample(0.5f);
        REQUIRE_FALSE(std::isnan(y));
        REQUIRE_FALSE(std::isinf(y));
        REQUIRE(std::fabs(y) < 10.0f);
    }
}

TEST_CASE("LowpassFilter: stable over long noisy input at high resonance", "[dsp][lpf]")
{
    LowpassFilter lpf;
    lpf.prepare(kSampleRate);
    lpf.setCutoff(200.0f);
    lpf.setResonance(10.0f);  // near the clamp upper limit

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-0.5f, 0.5f);

    for (int i = 0; i < 100000; ++i)
    {
        const float y = lpf.processSample(dist(rng));
        REQUIRE_FALSE(std::isnan(y));
        REQUIRE_FALSE(std::isinf(y));
        REQUIRE(std::fabs(y) < 100.0f);
    }
}

TEST_CASE("LowpassFilter: parameters set before prepare apply on prepare", "[dsp][lpf]")
{
    LowpassFilter lpf;
    lpf.setCutoff(1000.0f);
    lpf.setResonance(2.0f);

    for (int i = 0; i < 64; ++i)
        REQUIRE(lpf.processSample(1.0f) == 0.0f);

    lpf.prepare(kSampleRate);

    // Coefficients must actually be installed by prepare(): DC must
    // still pass at unity, not stay at zero or blow up.
    float out = 0.0f;
    for (int i = 0; i < 10000; ++i)
        out = lpf.processSample(1.0f);
    REQUIRE(out == Approx(1.0f).margin(1e-3f));
}

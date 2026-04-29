#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/Filter.h"

#include <vector>

#include <cmath>
#include <random>

using B33p::Filter;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr double kTwoPi      = 6.28318530717958647692;

    void warmupSine(Filter& lpf, double frequencyHz, int numSamples)
    {
        double phase = 0.0;
        const double phaseIncrement = kTwoPi * frequencyHz / kSampleRate;
        for (int i = 0; i < numSamples; ++i)
        {
            lpf.processSample(static_cast<float>(std::sin(phase)));
            phase += phaseIncrement;
        }
    }

    double rmsOfSineOutput(Filter& lpf, double frequencyHz, int numSamples)
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

TEST_CASE("Filter: processSample without prepare returns silence", "[dsp][lpf]")
{
    Filter lpf;
    lpf.setCutoff(1000.0f);
    lpf.setResonance(1.0f);

    for (int i = 0; i < 64; ++i)
        REQUIRE(lpf.processSample(1.0f) == 0.0f);
}

TEST_CASE("Filter: reset clears filter memory", "[dsp][lpf]")
{
    Filter lpf;
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

TEST_CASE("Filter: DC input passes through at unity gain", "[dsp][lpf]")
{
    Filter lpf;
    lpf.prepare(kSampleRate);
    lpf.setCutoff(1000.0f);
    lpf.setResonance(0.707f);

    float out = 0.0f;
    for (int i = 0; i < 10000; ++i)
        out = lpf.processSample(1.0f);

    // Biquad lowpass DC gain is exactly 1.0 by construction.
    REQUIRE(out == Approx(1.0f).margin(1e-3f));
}

TEST_CASE("Filter: in-passband sine passes with near-unity gain", "[dsp][lpf]")
{
    Filter lpf;
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

TEST_CASE("Filter: far-stopband sine is strongly attenuated", "[dsp][lpf]")
{
    Filter lpf;
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

TEST_CASE("Filter: higher resonance Q amplifies at cutoff frequency", "[dsp][lpf]")
{
    auto rmsAtCutoff = [](float q)
    {
        Filter lpf;
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

TEST_CASE("Filter: cutoff well above Nyquist stays stable", "[dsp][lpf]")
{
    Filter lpf;
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

TEST_CASE("Filter: stable over long noisy input at high resonance", "[dsp][lpf]")
{
    Filter lpf;
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

TEST_CASE("Filter: parameters set before prepare apply on prepare", "[dsp][lpf]")
{
    Filter lpf;
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

TEST_CASE("Filter: highpass strongly attenuates DC", "[dsp][filter]")
{
    Filter f;
    f.prepare(kSampleRate);
    f.setType(Filter::Type::Highpass);
    f.setCutoff(1000.0f);
    f.setResonance(0.707f);

    // Drive with a long DC signal and look at the steady-state
    // output — a 12 dB/oct highpass should have effectively zero
    // gain at DC.
    float steadyAbs = 0.0f;
    for (int i = 0; i < 4096; ++i)
    {
        const float y = f.processSample(1.0f);
        if (i >= 4000)
            steadyAbs = std::max(steadyAbs, std::abs(y));
    }
    REQUIRE(steadyAbs < 1e-3f);
}

TEST_CASE("Filter: bandpass passes its centre frequency more than far-stopband", "[dsp][filter]")
{
    Filter centre;
    centre.prepare(kSampleRate);
    centre.setType(Filter::Type::Bandpass);
    centre.setCutoff(1000.0f);
    centre.setResonance(2.0f);

    Filter offBand;
    offBand.prepare(kSampleRate);
    offBand.setType(Filter::Type::Bandpass);
    offBand.setCutoff(1000.0f);
    offBand.setResonance(2.0f);

    warmupSine(centre,  1000.0,  4000);
    warmupSine(offBand, 8000.0,  4000);

    const double rmsCentre = rmsOfSineOutput(centre,  1000.0, 2000);
    const double rmsOff    = rmsOfSineOutput(offBand, 8000.0, 2000);

    // Centre should pass at near unity; far-out frequency should be
    // attenuated heavily. Allow a generous margin since the BP slope
    // is gentle at Q=2.
    REQUIRE(rmsCentre > 0.5);
    REQUIRE(rmsOff    < 0.2);
    REQUIRE(rmsCentre > rmsOff * 3.0);
}

TEST_CASE("Filter: comb produces periodic peaks at its fundamental", "[dsp][filter]")
{
    // Drive a feedback comb at high feedback with two sines: one
    // at the comb's fundamental (peak), one at the fundamental ×
    // 1.5 (notch / mid-tooth). The peak frequency should ring
    // significantly louder than the off-tooth one.
    constexpr float fundamentalHz = 500.0f;

    Filter onTooth;
    onTooth.prepare(kSampleRate);
    onTooth.setType(Filter::Type::Comb);
    onTooth.setCutoff(fundamentalHz);
    onTooth.setResonance(15.0f);   // close to max feedback

    Filter offTooth;
    offTooth.prepare(kSampleRate);
    offTooth.setType(Filter::Type::Comb);
    offTooth.setCutoff(fundamentalHz);
    offTooth.setResonance(15.0f);

    warmupSine(onTooth,  static_cast<double>(fundamentalHz),       4000);
    warmupSine(offTooth, static_cast<double>(fundamentalHz) * 1.5, 4000);

    const double rmsOn  = rmsOfSineOutput(onTooth,
                                           static_cast<double>(fundamentalHz),       4000);
    const double rmsOff = rmsOfSineOutput(offTooth,
                                           static_cast<double>(fundamentalHz) * 1.5, 4000);

    REQUIRE(rmsOn > rmsOff * 2.0);
}

TEST_CASE("Filter: formant emphasises a vowel's first formant", "[dsp][filter]")
{
    // For vowel A (vowel01 = 0), F1 = 700 Hz. A 700 Hz sine
    // should pass through with substantially more energy than a
    // 1500 Hz sine that sits between F1 and F2.
    Filter onF1;
    onF1.prepare(kSampleRate);
    onF1.setType(Filter::Type::Formant);
    onF1.setVowel(0.0f);

    Filter offF1;
    offF1.prepare(kSampleRate);
    offF1.setType(Filter::Type::Formant);
    offF1.setVowel(0.0f);

    warmupSine(onF1,  700.0,  4000);
    warmupSine(offF1, 1500.0, 4000);

    const double rmsOn  = rmsOfSineOutput(onF1,  700.0,  2000);
    const double rmsOff = rmsOfSineOutput(offF1, 1500.0, 2000);

    REQUIRE(rmsOn > rmsOff);
}

TEST_CASE("Filter: changing type clears delay-line memory cleanly", "[dsp][filter]")
{
    // Pump comb mode at high feedback so the delay buffer fills,
    // then switch to lowpass. The lowpass output for a brand-new
    // input shouldn't leak any of the comb's prior memory.
    Filter f;
    f.prepare(kSampleRate);
    f.setType(Filter::Type::Comb);
    f.setCutoff(500.0f);
    f.setResonance(15.0f);
    warmupSine(f, 500.0, 8000);

    f.setType(Filter::Type::Lowpass);
    f.setCutoff(20000.0f);
    f.setResonance(0.707f);
    f.reset();

    // After reset, processing zero input should produce zero output.
    for (int i = 0; i < 256; ++i)
        REQUIRE(f.processSample(0.0f) == Approx(0.0f).margin(1e-6f));
}

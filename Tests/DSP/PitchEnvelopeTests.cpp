#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/PitchEnvelope.h"

#include <cmath>
#include <vector>

using B33p::PitchEnvelope;
using B33p::PitchEnvelopePoint;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;

    std::vector<float> render(PitchEnvelope& env, int numSamples)
    {
        std::vector<float> out(static_cast<size_t>(numSamples));
        for (int i = 0; i < numSamples; ++i)
            out[static_cast<size_t>(i)] = env.processSample();
        return out;
    }

    // Search a small window around an expected breakpoint index for a
    // sample within tolerance. Matches the DSP convention: phase
    // accumulators drift sub-sample, so boundary events are checked with
    // a tolerance window rather than exact equality.
    bool hasValueWithin(const std::vector<float>& samples,
                        int centerIndex,
                        int halfWindow,
                        float expected,
                        float tolerance)
    {
        const int lo = std::max(0, centerIndex - halfWindow);
        const int hi = std::min(static_cast<int>(samples.size()) - 1,
                                centerIndex + halfWindow);
        for (int i = lo; i <= hi; ++i)
            if (std::fabs(samples[static_cast<size_t>(i)] - expected) <= tolerance)
                return true;
        return false;
    }
}

TEST_CASE("PitchEnvelope: processSample without prepare returns silence", "[dsp][pitchenv]")
{
    PitchEnvelope env;
    env.setCurve({ { 0.0f, -5.0f }, { 1.0f, 5.0f } });

    env.trigger(0.01f);  // guarded: should not transition out of Idle before prepare
    REQUIRE_FALSE(env.isActive());

    for (int i = 0; i < 64; ++i)
        REQUIRE(env.processSample() == 0.0f);
}

TEST_CASE("PitchEnvelope: prepare leaves envelope idle and silent until trigger", "[dsp][pitchenv]")
{
    PitchEnvelope env;
    env.prepare(kSampleRate);
    env.setCurve({ { 0.0f, -5.0f }, { 1.0f, 5.0f } });

    REQUIRE_FALSE(env.isActive());

    for (int i = 0; i < 64; ++i)
        REQUIRE(env.processSample() == 0.0f);
}

TEST_CASE("PitchEnvelope: reset returns envelope to idle and silent", "[dsp][pitchenv]")
{
    PitchEnvelope env;
    env.prepare(kSampleRate);
    env.setCurve({ { 0.0f, -5.0f }, { 1.0f, 5.0f } });

    env.trigger(0.1f);
    for (int i = 0; i < 100; ++i)
        (void) env.processSample();

    REQUIRE(env.isActive());

    env.reset();
    REQUIRE_FALSE(env.isActive());

    for (int i = 0; i < 64; ++i)
        REQUIRE(env.processSample() == 0.0f);
}

TEST_CASE("PitchEnvelope: flat curve outputs constant value throughout", "[dsp][pitchenv]")
{
    PitchEnvelope env;
    env.prepare(kSampleRate);
    env.setCurve({ { 0.0f, 3.5f }, { 0.5f, 3.5f }, { 1.0f, 3.5f } });

    env.trigger(0.05f);  // 2400 samples

    const std::vector<float> samples = render(env, 3000);
    for (float s : samples)
        REQUIRE(s == Approx(3.5f).margin(1e-5));
}

TEST_CASE("PitchEnvelope: two-point linear ramp hits midpoint at center sample", "[dsp][pitchenv]")
{
    PitchEnvelope env;
    env.prepare(kSampleRate);
    env.setCurve({ { 0.0f, -12.0f }, { 1.0f, 12.0f } });

    env.trigger(1.0f);  // 48000 samples

    const std::vector<float> samples = render(env, 48001);

    // First sample reads t=0 exactly -> -12 semitones.
    REQUIRE(samples.front() == Approx(-12.0f).margin(1e-4));

    // Midpoint: sample 24000 reads t = 24000/48000 = 0.5 -> 0 semitones.
    // Double accumulator drift is well under a cent over 24000 steps.
    REQUIRE(samples[24000] == Approx(0.0f).margin(0.01f));

    // Last rendered sample is in the Holding stage, clamped to +12.
    REQUIRE(samples.back() == Approx(12.0f).margin(1e-4));
}

TEST_CASE("PitchEnvelope: multi-point curve passes through each breakpoint", "[dsp][pitchenv]")
{
    PitchEnvelope env;
    env.prepare(kSampleRate);
    env.setCurve({
        { 0.0f,   0.0f },
        { 0.25f,  5.0f },
        { 0.75f, -5.0f },
        { 1.0f,   0.0f }
    });

    env.trigger(1.0f);  // 48000 samples

    const std::vector<float> samples = render(env, 48000);

    // Within a three-sample window of each breakpoint, at least one
    // sample must land within 0.01 semitones of the expected value.
    REQUIRE(hasValueWithin(samples,     0, 3,  0.0f, 0.01f));
    REQUIRE(hasValueWithin(samples, 12000, 3,  5.0f, 0.01f));
    REQUIRE(hasValueWithin(samples, 36000, 3, -5.0f, 0.01f));
}

TEST_CASE("PitchEnvelope: duration scaling plays the same curve at different rates", "[dsp][pitchenv]")
{
    const std::vector<PitchEnvelopePoint> curve = { { 0.0f, 0.0f }, { 1.0f, 10.0f } };

    SECTION("0.1 second trigger finishes in ~4800 samples")
    {
        PitchEnvelope env;
        env.prepare(kSampleRate);
        env.setCurve(curve);
        env.trigger(0.1f);

        for (int i = 0; i < 4700; ++i)
            (void) env.processSample();
        REQUIRE(env.isActive());

        for (int i = 0; i < 200; ++i)
            (void) env.processSample();
        REQUIRE_FALSE(env.isActive());
    }

    SECTION("1.0 second trigger is still active at 4800 samples")
    {
        PitchEnvelope env;
        env.prepare(kSampleRate);
        env.setCurve(curve);
        env.trigger(1.0f);

        for (int i = 0; i < 4800; ++i)
            (void) env.processSample();
        REQUIRE(env.isActive());
    }
}

TEST_CASE("PitchEnvelope: holds last point value after duration elapses", "[dsp][pitchenv]")
{
    PitchEnvelope env;
    env.prepare(kSampleRate);
    env.setCurve({ { 0.0f, -5.0f }, { 1.0f, 7.0f } });

    env.trigger(0.01f);  // 480 samples

    const std::vector<float> samples = render(env, 1000);

    REQUIRE_FALSE(env.isActive());

    // Tail samples (well past the 480-sample duration) all hold at +7.
    for (size_t i = 700; i < samples.size(); ++i)
        REQUIRE(samples[i] == Approx(7.0f).margin(1e-4));
}

TEST_CASE("PitchEnvelope: retrigger mid-playback restarts from curve start", "[dsp][pitchenv]")
{
    PitchEnvelope env;
    env.prepare(kSampleRate);
    env.setCurve({ { 0.0f, -10.0f }, { 1.0f, 10.0f } });

    env.trigger(1.0f);  // 48000 samples

    // Advance halfway: output should be near 0.
    for (int i = 0; i < 24000; ++i)
        (void) env.processSample();

    env.trigger(1.0f);

    const float first = env.processSample();
    REQUIRE(first == Approx(-10.0f).margin(1e-4));
    REQUIRE(env.isActive());
}

TEST_CASE("PitchEnvelope: empty curve yields constant zero offset", "[dsp][pitchenv]")
{
    PitchEnvelope env;
    env.prepare(kSampleRate);
    env.setCurve({});

    env.trigger(0.05f);

    const std::vector<float> samples = render(env, 3000);
    for (float s : samples)
        REQUIRE(s == Approx(0.0f).margin(1e-6));
}

TEST_CASE("PitchEnvelope: single-point curve yields that constant value", "[dsp][pitchenv]")
{
    PitchEnvelope env;
    env.prepare(kSampleRate);
    env.setCurve({ { 0.3f, 4.25f } });

    env.trigger(0.05f);

    const std::vector<float> samples = render(env, 3000);
    for (float s : samples)
        REQUIRE(s == Approx(4.25f).margin(1e-5));
}

TEST_CASE("PitchEnvelope: setCurve with out-of-order points sorts and produces monotonic output", "[dsp][pitchenv]")
{
    PitchEnvelope env;
    env.prepare(kSampleRate);

    // Intentionally out of order. After sort: (0,0), (0.5,5), (1,10).
    env.setCurve({
        { 0.5f,  5.0f },
        { 0.0f,  0.0f },
        { 1.0f, 10.0f }
    });

    env.trigger(0.1f);  // 4800 samples

    const std::vector<float> samples = render(env, 4800);

    // If the sort worked, output progresses monotonically from 0 to 10.
    // Use a tiny epsilon so float-drift equality doesn't fail the check.
    for (size_t i = 1; i < samples.size(); ++i)
        REQUIRE(samples[i] >= samples[i - 1] - 1e-5f);

    REQUIRE(samples.front() == Approx(0.0f).margin(1e-4));
    REQUIRE(samples.back()  == Approx(10.0f).margin(0.01f));
}

TEST_CASE("PitchEnvelope: setCurve clamps out-of-range times into [0, 1]", "[dsp][pitchenv]")
{
    PitchEnvelope env;
    env.prepare(kSampleRate);

    // t=1.5 should clamp to 1.0. After clamp the curve spans [0, 1] with
    // endpoints at 0 and 10, so the midpoint sample must read ~5.
    env.setCurve({ { 0.0f, 0.0f }, { 1.5f, 10.0f } });

    env.trigger(1.0f);

    const std::vector<float> samples = render(env, 48001);

    REQUIRE(samples[24000] == Approx(5.0f).margin(0.01f));
    REQUIRE(samples.back()  == Approx(10.0f).margin(1e-4));
}

TEST_CASE("PitchEnvelope: trigger(0) holds last point on the first sample", "[dsp][pitchenv]")
{
    PitchEnvelope env;
    env.prepare(kSampleRate);
    env.setCurve({ { 0.0f, -3.0f }, { 1.0f, 7.0f } });

    env.trigger(0.0f);

    // "Curve has already finished" — first sample is the last point's
    // value, not the first point's. No one-sample glitch of -3.
    const float first = env.processSample();
    REQUIRE(first == Approx(7.0f).margin(1e-4));
    REQUIRE_FALSE(env.isActive());

    for (int i = 0; i < 100; ++i)
        REQUIRE(env.processSample() == Approx(7.0f).margin(1e-4));
}

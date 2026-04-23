#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/Bitcrush.h"

#include <cmath>
#include <random>
#include <vector>

using B33p::Bitcrush;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr double kTwoPi      = 6.28318530717958647692;
}

TEST_CASE("Bitcrush: processSample without prepare returns silence", "[dsp][bitcrush]")
{
    Bitcrush bc;
    bc.setBitDepth(4.0f);
    bc.setTargetSampleRate(8000.0f);

    for (int i = 0; i < 64; ++i)
        REQUIRE(bc.processSample(0.5f) == 0.0f);
}

TEST_CASE("Bitcrush: 16-bit at host rate is near-transparent to a sine", "[dsp][bitcrush]")
{
    Bitcrush bc;
    bc.prepare(kSampleRate);
    bc.setBitDepth(16.0f);
    bc.setTargetSampleRate(static_cast<float>(kSampleRate));

    // Max expected error: one quantization step (2 / 2^16).
    const float tolerance = 2.0f / 65536.0f + 1e-6f;

    double phase = 0.0;
    const double phaseIncrement = kTwoPi * 440.0 / kSampleRate;

    for (int i = 0; i < 1000; ++i)
    {
        const float x = static_cast<float>(std::sin(phase));
        phase += phaseIncrement;

        const float y = bc.processSample(x);
        REQUIRE(std::fabs(y - x) <= tolerance);
    }
}

TEST_CASE("Bitcrush: 1-bit output collapses to {-1, 0, +1}", "[dsp][bitcrush]")
{
    Bitcrush bc;
    bc.prepare(kSampleRate);
    bc.setBitDepth(1.0f);
    bc.setTargetSampleRate(static_cast<float>(kSampleRate));

    for (int i = 0; i < 200; ++i)
    {
        const float x = -1.0f + static_cast<float>(i) * (2.0f / 199.0f);
        const float y = bc.processSample(x);

        const bool onGrid =
               std::fabs(y - (-1.0f)) < 1e-5f
            || std::fabs(y -   0.0f ) < 1e-5f
            || std::fabs(y -   1.0f ) < 1e-5f;
        REQUIRE(onGrid);
    }
}

TEST_CASE("Bitcrush: 2-bit output lands on 0.5-unit grid", "[dsp][bitcrush]")
{
    Bitcrush bc;
    bc.prepare(kSampleRate);
    bc.setBitDepth(2.0f);
    bc.setTargetSampleRate(static_cast<float>(kSampleRate));

    for (int i = 0; i < 200; ++i)
    {
        const float x = -1.0f + static_cast<float>(i) * (2.0f / 199.0f);
        const float y = bc.processSample(x);

        const float ratio = y / 0.5f;
        REQUIRE(std::fabs(ratio - std::round(ratio)) < 1e-5f);
    }
}

TEST_CASE("Bitcrush: SR reduction holds samples for target period", "[dsp][bitcrush]")
{
    Bitcrush bc;
    bc.prepare(kSampleRate);
    bc.setBitDepth(16.0f);
    bc.setTargetSampleRate(12000.0f);   // 4:1 reduction

    std::vector<float> out;
    out.reserve(100);
    for (int i = 0; i < 100; ++i)
        out.push_back(bc.processSample(static_cast<float>(i) * 0.001f));

    // Expect runs of 4 identical outputs. Tolerance window ±1 for
    // any boundary edge case in the phase accumulator.
    int runLength = 1;
    int maxRun    = 1;
    for (size_t i = 1; i < out.size(); ++i)
    {
        if (std::fabs(out[i] - out[i - 1]) < 1e-6f)
        {
            ++runLength;
            maxRun = std::max(maxRun, runLength);
        }
        else
        {
            runLength = 1;
        }
    }

    REQUIRE(maxRun >= 3);
    REQUIRE(maxRun <= 5);
}

TEST_CASE("Bitcrush: reset clears held sample and forces a fresh capture", "[dsp][bitcrush]")
{
    Bitcrush bc;
    bc.prepare(kSampleRate);
    bc.setBitDepth(16.0f);
    bc.setTargetSampleRate(12000.0f);   // 4:1

    const float firstCapture = bc.processSample(0.5f);
    REQUIRE(firstCapture == Approx(0.5f).margin(2.0f / 65536.0f));

    // Held for the next three samples even when input changes.
    for (int i = 0; i < 3; ++i)
        REQUIRE(bc.processSample(0.999f) == Approx(firstCapture).margin(1e-6f));

    bc.reset();

    // After reset, the next sample must capture the new input.
    const float afterReset = bc.processSample(-0.3f);
    REQUIRE(afterReset == Approx(-0.3f).margin(2.0f / 65536.0f));
}

TEST_CASE("Bitcrush: target above host rate captures every input sample", "[dsp][bitcrush]")
{
    Bitcrush bc;
    bc.prepare(kSampleRate);
    bc.setBitDepth(16.0f);
    bc.setTargetSampleRate(96000.0f);   // 2x host

    const float tolerance = 2.0f / 65536.0f + 1e-6f;

    for (int i = 0; i < 100; ++i)
    {
        const float x = std::sin(static_cast<float>(i) * 0.1f);
        const float y = bc.processSample(x);
        REQUIRE(std::fabs(y - x) <= tolerance);
    }
}

TEST_CASE("Bitcrush: stable over long noisy input at extreme settings", "[dsp][bitcrush]")
{
    Bitcrush bc;
    bc.prepare(kSampleRate);
    bc.setBitDepth(1.0f);
    bc.setTargetSampleRate(20.0f);   // lower clamp

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    for (int i = 0; i < 100000; ++i)
    {
        const float y = bc.processSample(dist(rng));
        REQUIRE_FALSE(std::isnan(y));
        REQUIRE_FALSE(std::isinf(y));
        REQUIRE(std::fabs(y) <= 1.0f + 1e-5f);
    }
}

TEST_CASE("Bitcrush: out-of-range parameters clamp without crashing", "[dsp][bitcrush]")
{
    Bitcrush bc;
    bc.prepare(kSampleRate);

    bc.setBitDepth(0.5f);                // below [1, 16] lower clamp
    bc.setTargetSampleRate(-1000.0f);    // below 20 Hz lower clamp
    for (int i = 0; i < 1000; ++i)
    {
        const float y = bc.processSample(0.5f);
        REQUIRE_FALSE(std::isnan(y));
        REQUIRE_FALSE(std::isinf(y));
    }

    bc.setBitDepth(99.0f);               // above upper clamp
    bc.setTargetSampleRate(1.0e9f);      // far above host rate
    for (int i = 0; i < 1000; ++i)
    {
        const float y = bc.processSample(0.5f);
        REQUIRE_FALSE(std::isnan(y));
        REQUIRE_FALSE(std::isinf(y));
    }
}

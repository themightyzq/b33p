#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/OutputLimiter.h"

#include <cmath>
#include <random>

using B33p::OutputLimiter;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;
}

TEST_CASE("OutputLimiter: processSample without prepare returns silence", "[dsp][limiter]")
{
    OutputLimiter lim;
    for (int i = 0; i < 64; ++i)
        REQUIRE(lim.processSample(0.5f) == 0.0f);
}

TEST_CASE("OutputLimiter: clean signal below the knee passes untouched", "[dsp][limiter]")
{
    OutputLimiter lim;
    lim.prepare(kSampleRate);

    // |x| <= knee is the identity region — bit-exact passthrough.
    for (int i = -70; i <= 70; ++i)
    {
        const float x = static_cast<float>(i) * 0.01f;   // -0.70 .. 0.70
        REQUIRE(lim.processSample(x) == x);
    }
}

TEST_CASE("OutputLimiter: transfer is continuous and C1 at the knee", "[dsp][limiter]")
{
    OutputLimiter lim;
    lim.prepare(kSampleRate);

    // Value continuity: at the knee the soft region equals the identity
    // value (both are `knee`).
    REQUIRE(lim.processSample(OutputLimiter::kKnee)
                == Approx(OutputLimiter::kKnee).margin(1e-6f));

    // Slope continuity: the soft-knee starts at unit slope, so a small
    // step just above the knee advances the output by ~the same step.
    const float delta = 1.0e-4f;
    const float y0    = lim.processSample(OutputLimiter::kKnee);
    const float y1    = lim.processSample(OutputLimiter::kKnee + delta);
    REQUIRE((y1 - y0) / delta == Approx(1.0f).margin(1e-3f));
}

TEST_CASE("OutputLimiter: output stays at or below the ceiling (never hard-clips)", "[dsp][limiter]")
{
    OutputLimiter lim;
    lim.prepare(kSampleRate);

    // Anything past the knee — including absurd overloads — is rounded
    // toward the ceiling. (For huge inputs exp() underflows to 0 in float,
    // so the result rounds up to exactly the ceiling.) Either way it stays
    // below 0 dBFS, so the summed output can't hit full-scale and trip the
    // meter's clip latch (which fires at >= 0.999).
    const float overloads[] = { 0.8f, 1.0f, 2.0f, 10.0f, 1000.0f };
    for (float x : overloads)
    {
        const float y = lim.processSample(x);
        REQUIRE(y <= OutputLimiter::kCeiling);
        REQUIRE(y < 1.0f);
        REQUIRE(y > OutputLimiter::kKnee);   // still above the linear region
    }
}

TEST_CASE("OutputLimiter: transfer function is odd (f(-x) == -f(x))", "[dsp][limiter]")
{
    OutputLimiter lim;
    lim.prepare(kSampleRate);

    for (int i = 1; i <= 200; ++i)
    {
        const float x = static_cast<float>(i) * 0.02f;   // spans the knee
        REQUIRE(lim.processSample(x) + lim.processSample(-x)
                    == Approx(0.0f).margin(1e-6f));
    }
}

TEST_CASE("OutputLimiter: transfer is monotonically non-decreasing", "[dsp][limiter]")
{
    OutputLimiter lim;
    lim.prepare(kSampleRate);

    float prev = lim.processSample(-5.0f);
    for (int i = -499; i <= 500; ++i)
    {
        const float y = lim.processSample(static_cast<float>(i) * 0.01f);
        REQUIRE(y >= prev - 1e-6f);
        prev = y;
    }
}

TEST_CASE("OutputLimiter: bounded + finite under long noisy overload", "[dsp][limiter]")
{
    OutputLimiter lim;
    lim.prepare(kSampleRate);

    std::mt19937 rng(7);
    std::uniform_real_distribution<float> uniform(-8.0f, 8.0f);

    for (int i = 0; i < 100000; ++i)
    {
        const float y = lim.processSample(uniform(rng));
        REQUIRE_FALSE(std::isnan(y));
        REQUIRE_FALSE(std::isinf(y));
        REQUIRE(std::fabs(y) <= OutputLimiter::kCeiling);
    }
}

TEST_CASE("OutputLimiter: reset is a no-op on a memoryless stage", "[dsp][limiter]")
{
    OutputLimiter lim;
    lim.prepare(kSampleRate);

    const float before = lim.processSample(0.9f);
    lim.reset();
    const float after  = lim.processSample(0.9f);
    REQUIRE(before == Approx(after).margin(1e-6f));
}

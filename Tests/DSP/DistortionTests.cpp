#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/Distortion.h"

#include <cmath>
#include <random>

using B33p::Distortion;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;
}

TEST_CASE("Distortion: processSample without prepare returns silence", "[dsp][distortion]")
{
    Distortion dist;
    dist.setDrive(5.0f);

    for (int i = 0; i < 64; ++i)
        REQUIRE(dist.processSample(0.5f) == 0.0f);
}

TEST_CASE("Distortion: small signal at drive=1 passes near-linearly", "[dsp][distortion]")
{
    Distortion dist;
    dist.prepare(kSampleRate);
    dist.setDrive(1.0f);

    // For |x| <= 0.1, the tanh(x) ~ x - x^3/3 expansion keeps the error
    // below 4e-4.
    for (int i = -100; i <= 100; ++i)
    {
        const float x = static_cast<float>(i) * 0.001f;
        const float y = dist.processSample(x);
        REQUIRE(std::fabs(y - x) < 1e-3f);
    }
}

TEST_CASE("Distortion: high drive saturates peaks toward unit amplitude", "[dsp][distortion]")
{
    Distortion dist;
    dist.prepare(kSampleRate);
    dist.setDrive(50.0f);

    REQUIRE(dist.processSample( 1.0f) == Approx( 1.0f).margin(1e-6f));
    REQUIRE(dist.processSample(-1.0f) == Approx(-1.0f).margin(1e-6f));
    REQUIRE(dist.processSample( 0.5f) >  0.99f);
    REQUIRE(dist.processSample(-0.5f) < -0.99f);
}

TEST_CASE("Distortion: output is bounded in [-1, 1] for any finite input", "[dsp][distortion]")
{
    Distortion dist;
    dist.prepare(kSampleRate);
    dist.setDrive(100.0f);

    const float extremes[] = { -1000.0f, -1.0f, 0.0f, 1.0f, 1000.0f };
    for (float x : extremes)
    {
        const float y = dist.processSample(x);
        REQUIRE_FALSE(std::isnan(y));
        REQUIRE_FALSE(std::isinf(y));
        REQUIRE(std::fabs(y) <= 1.0f);
    }
}

TEST_CASE("Distortion: transfer function is odd (f(-x) == -f(x))", "[dsp][distortion]")
{
    Distortion dist;
    dist.prepare(kSampleRate);
    dist.setDrive(3.7f);

    for (int i = 1; i <= 50; ++i)
    {
        const float x    = static_cast<float>(i) * 0.02f;
        const float yPos = dist.processSample( x);
        const float yNeg = dist.processSample(-x);
        REQUIRE(yPos + yNeg == Approx(0.0f).margin(1e-6f));
    }
}

TEST_CASE("Distortion: DC input produces matching constant output", "[dsp][distortion]")
{
    Distortion dist;
    dist.prepare(kSampleRate);
    dist.setDrive(2.0f);

    const float expected = std::tanh(2.0f * 0.5f);
    for (int i = 0; i < 1000; ++i)
        REQUIRE(dist.processSample(0.5f) == Approx(expected).margin(1e-6f));
}

TEST_CASE("Distortion: drive clamps to [0.1, 100]", "[dsp][distortion]")
{
    Distortion dist;
    dist.prepare(kSampleRate);

    dist.setDrive(-5.0f);   // below lower clamp -> 0.1
    REQUIRE(dist.processSample(1.0f) == Approx(std::tanh(0.1f)).margin(1e-6f));

    dist.setDrive(1000.0f); // above upper clamp -> 100
    REQUIRE(dist.processSample(1.0f) == Approx(1.0f).margin(1e-6f));
}

TEST_CASE("Distortion: stable under long noisy input at extreme drive", "[dsp][distortion]")
{
    Distortion dist;
    dist.prepare(kSampleRate);
    dist.setDrive(100.0f);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> uniform(-1.0f, 1.0f);

    for (int i = 0; i < 100000; ++i)
    {
        const float y = dist.processSample(uniform(rng));
        REQUIRE_FALSE(std::isnan(y));
        REQUIRE_FALSE(std::isinf(y));
        REQUIRE(std::fabs(y) <= 1.0f);
    }
}

TEST_CASE("Distortion: reset is a no-op on a memoryless shaper", "[dsp][distortion]")
{
    Distortion dist;
    dist.prepare(kSampleRate);
    dist.setDrive(5.0f);

    const float before = dist.processSample(0.3f);
    dist.reset();
    const float after  = dist.processSample(0.3f);
    REQUIRE(before == Approx(after).margin(1e-6f));
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/LFO.h"

#include <cmath>
#include <vector>

using B33p::LFO;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;
}

TEST_CASE("LFO: currentValue without prepare returns 0", "[dsp][lfo]")
{
    LFO l;
    l.setShape(LFO::Shape::Sine);
    l.setRate(2.0f);

    REQUIRE(l.currentValue() == Approx(0.0f).margin(1e-6f));
}

TEST_CASE("LFO: rate of zero never advances the phase", "[dsp][lfo]")
{
    LFO l;
    l.prepare(kSampleRate);
    l.setShape(LFO::Shape::Sine);
    l.setRate(0.0f);

    const float start = l.currentValue();
    for (int i = 0; i < 1000; ++i)
        l.advance(512);
    REQUIRE(l.currentValue() == Approx(start).margin(1e-6f));
}

TEST_CASE("LFO: completes one cycle after sampleRate / rate samples", "[dsp][lfo]")
{
    // Advance by exactly one period (sampleRate / rate samples)
    // and the LFO should land back at its starting phase / value.
    LFO l;
    l.prepare(kSampleRate);
    l.setShape(LFO::Shape::Sine);
    l.setRate(10.0f);

    const float start = l.currentValue();
    const int samplesPerPeriod = static_cast<int>(kSampleRate / 10.0);
    l.advance(samplesPerPeriod);

    // Allow a small margin because integer sample counts don't
    // divide evenly into the floating-point period.
    REQUIRE(l.currentValue() == Approx(start).margin(1e-3f));
}

TEST_CASE("LFO: square shape outputs +/- 1 only", "[dsp][lfo]")
{
    LFO l;
    l.prepare(kSampleRate);
    l.setShape(LFO::Shape::Square);
    l.setRate(5.0f);

    // Sample the LFO across many phases — every reading must be
    // exactly +1 or -1.
    for (int i = 0; i < 200; ++i)
    {
        const float v = l.currentValue();
        REQUIRE((v == Approx(1.0f) || v == Approx(-1.0f)));
        l.advance(50);
    }
}

TEST_CASE("LFO: shape switch changes output without re-preparing", "[dsp][lfo]")
{
    LFO l;
    l.prepare(kSampleRate);
    l.setRate(1.0f);

    l.setShape(LFO::Shape::Saw);
    const float sawValue = l.currentValue();

    l.setShape(LFO::Shape::Triangle);
    const float triValue = l.currentValue();

    // Saw at phase 0 = -1; Triangle at phase 0 = 0. They must
    // differ at the same phase.
    REQUIRE(sawValue == Approx(-1.0f).margin(1e-6f));
    REQUIRE(triValue == Approx( 0.0f).margin(1e-6f));
}

TEST_CASE("LFO: reset returns phase to zero", "[dsp][lfo]")
{
    LFO l;
    l.prepare(kSampleRate);
    l.setShape(LFO::Shape::Saw);
    l.setRate(5.0f);

    l.advance(10000);
    REQUIRE(l.currentValue() != Approx(-1.0f));

    l.reset();
    REQUIRE(l.currentValue() == Approx(-1.0f).margin(1e-6f));
}

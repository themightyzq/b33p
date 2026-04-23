#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/AmpEnvelope.h"

#include <cmath>
#include <vector>

using B33p::AmpEnvelope;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;

    std::vector<float> render(AmpEnvelope& env, int numSamples)
    {
        std::vector<float> out(static_cast<size_t>(numSamples));
        for (int i = 0; i < numSamples; ++i)
            out[static_cast<size_t>(i)] = env.processSample();
        return out;
    }

    int firstSampleAtOrAbove(const std::vector<float>& samples, float threshold)
    {
        for (size_t i = 0; i < samples.size(); ++i)
            if (samples[i] >= threshold)
                return static_cast<int>(i);
        return -1;
    }

    int firstSampleAtOrBelow(const std::vector<float>& samples, float threshold)
    {
        for (size_t i = 0; i < samples.size(); ++i)
            if (samples[i] <= threshold)
                return static_cast<int>(i);
        return -1;
    }
}

TEST_CASE("AmpEnvelope: processSample without prepare returns silence", "[dsp][ampenv]")
{
    AmpEnvelope env;
    env.setAttack(0.01f);
    env.setDecay(0.01f);
    env.setSustain(0.8f);
    env.setRelease(0.01f);

    env.noteOn();  // guarded: should not transition out of Idle before prepare
    REQUIRE_FALSE(env.isActive());

    for (int i = 0; i < 64; ++i)
        REQUIRE(env.processSample() == 0.0f);
}

TEST_CASE("AmpEnvelope: prepare leaves envelope idle and silent until noteOn", "[dsp][ampenv]")
{
    AmpEnvelope env;
    env.prepare(kSampleRate);
    REQUIRE_FALSE(env.isActive());

    for (int i = 0; i < 64; ++i)
        REQUIRE(env.processSample() == 0.0f);
}

TEST_CASE("AmpEnvelope: reset returns envelope to idle and silent", "[dsp][ampenv]")
{
    AmpEnvelope env;
    env.prepare(kSampleRate);
    env.setAttack(0.01f);
    env.setDecay(0.01f);
    env.setSustain(0.5f);
    env.setRelease(0.01f);

    env.noteOn();
    for (int i = 0; i < 100; ++i)
        (void) env.processSample();

    REQUIRE(env.isActive());

    env.reset();
    REQUIRE_FALSE(env.isActive());

    for (int i = 0; i < 64; ++i)
        REQUIRE(env.processSample() == 0.0f);
}

TEST_CASE("AmpEnvelope: attack ramps 0 to 1 over approximately attack seconds", "[dsp][ampenv]")
{
    AmpEnvelope env;
    env.prepare(kSampleRate);
    env.setAttack(0.01f);   // 480 samples at 48 kHz
    env.setDecay(1.0f);
    env.setSustain(0.0f);
    env.setRelease(1.0f);

    env.noteOn();

    const std::vector<float> samples = render(env, 520);

    REQUIRE(samples.front() >  0.0f);     // started ramping
    REQUIRE(samples.front() <  0.01f);    // started near zero
    REQUIRE_FALSE(std::isnan(samples.front()));

    // Peak (1.0) reached within a tolerance window around 480; phase-accumulator
    // drift from float rounding can nudge the crossing by a few samples either
    // way (DSP conventions: boundary events use a tolerance window).
    const int firstPeak = firstSampleAtOrAbove(samples, 1.0f);
    REQUIRE(firstPeak >= 477);
    REQUIRE(firstPeak <= 483);
}

TEST_CASE("AmpEnvelope: decay ramps 1 to sustain over approximately decay seconds", "[dsp][ampenv]")
{
    AmpEnvelope env;
    env.prepare(kSampleRate);
    env.setAttack(0.0f);     // snap to peak on first sample
    env.setDecay(0.01f);     // 480 samples
    env.setSustain(0.25f);
    env.setRelease(1.0f);

    env.noteOn();

    const float peak = env.processSample();
    REQUIRE(peak == Approx(1.0f).margin(1e-6));

    const std::vector<float> samples = render(env, 520);

    const int firstAtSustain = firstSampleAtOrBelow(samples, 0.25f);
    REQUIRE(firstAtSustain >= 477);
    REQUIRE(firstAtSustain <= 483);

    // Stays at sustain afterwards.
    for (size_t i = static_cast<size_t>(firstAtSustain); i < samples.size(); ++i)
        REQUIRE(samples[i] == Approx(0.25f).margin(1e-5));
}

TEST_CASE("AmpEnvelope: sustain holds indefinitely until noteOff", "[dsp][ampenv]")
{
    AmpEnvelope env;
    env.prepare(kSampleRate);
    env.setAttack(0.0f);
    env.setDecay(0.0f);
    env.setSustain(0.6f);
    env.setRelease(0.01f);

    env.noteOn();

    // A few samples for attack/decay snaps to settle.
    for (int i = 0; i < 4; ++i)
        (void) env.processSample();

    // Hold for two seconds of audio and verify the level never drifts.
    const std::vector<float> samples = render(env, 96000);
    for (float s : samples)
        REQUIRE(s == Approx(0.6f).margin(1e-5));

    REQUIRE(env.isActive());
}

TEST_CASE("AmpEnvelope: release ramps from sustain to zero over approximately release seconds", "[dsp][ampenv]")
{
    AmpEnvelope env;
    env.prepare(kSampleRate);
    env.setAttack(0.0f);
    env.setDecay(0.0f);
    env.setSustain(0.5f);
    env.setRelease(0.01f);  // 480 samples

    env.noteOn();
    (void) env.processSample();  // attack snap
    (void) env.processSample();  // decay snap → sustain

    env.noteOff();

    const std::vector<float> samples = render(env, 520);

    const int firstAtZero = firstSampleAtOrBelow(samples, 0.0f);
    REQUIRE(firstAtZero >= 477);
    REQUIRE(firstAtZero <= 483);
    REQUIRE_FALSE(env.isActive());
}

TEST_CASE("AmpEnvelope: retrigger mid-decay restarts attack from current level", "[dsp][ampenv]")
{
    AmpEnvelope env;
    env.prepare(kSampleRate);
    env.setAttack(0.01f);
    env.setDecay(0.1f);      // 4800 samples — easy to catch mid-decay
    env.setSustain(0.0f);
    env.setRelease(0.01f);

    env.noteOn();

    constexpr int preRetrigger = 1000;
    std::vector<float> before;
    before.reserve(preRetrigger);
    for (int i = 0; i < preRetrigger; ++i)
        before.push_back(env.processSample());

    const float lastBefore = before.back();
    REQUIRE(lastBefore > 0.1f);   // unambiguously mid-decay, not near zero
    REQUIRE(lastBefore < 1.0f);   // not still at peak

    env.noteOn();

    const float firstAfter = env.processSample();

    // No click: sample-to-sample delta must be dominated by the new attack
    // rate, not a snap to zero. Attack rate from mid-decay level is well under
    // 0.01 per sample; a snap to zero would be > 0.8.
    const float delta = std::fabs(firstAfter - lastBefore);
    REQUIRE(delta < 0.05f);

    // And the attack should continue ramping upward toward 1.0.
    REQUIRE(firstAfter > lastBefore);
}

TEST_CASE("AmpEnvelope: zero-length attack snaps to peak in one sample", "[dsp][ampenv]")
{
    AmpEnvelope env;
    env.prepare(kSampleRate);
    env.setAttack(0.0f);
    env.setDecay(1.0f);
    env.setSustain(1.0f);
    env.setRelease(1.0f);

    env.noteOn();

    const float first = env.processSample();
    REQUIRE_FALSE(std::isnan(first));
    REQUIRE(first == Approx(1.0f).margin(1e-6));
}

TEST_CASE("AmpEnvelope: zero-length decay snaps to sustain in one sample", "[dsp][ampenv]")
{
    AmpEnvelope env;
    env.prepare(kSampleRate);
    env.setAttack(0.0f);
    env.setDecay(0.0f);
    env.setSustain(0.3f);
    env.setRelease(1.0f);

    env.noteOn();

    const float s0 = env.processSample();   // attack snap to 1.0
    REQUIRE(s0 == Approx(1.0f).margin(1e-6));

    const float s1 = env.processSample();   // decay snap to sustain
    REQUIRE_FALSE(std::isnan(s1));
    REQUIRE(s1 == Approx(0.3f).margin(1e-6));

    const float s2 = env.processSample();   // still at sustain
    REQUIRE(s2 == Approx(0.3f).margin(1e-6));
}

TEST_CASE("AmpEnvelope: zero sustain with finite release reaches exactly zero and goes idle", "[dsp][ampenv]")
{
    AmpEnvelope env;
    env.prepare(kSampleRate);
    env.setAttack(0.0f);
    env.setDecay(1.0f);      // long decay so we noteOff while level > 0
    env.setSustain(0.0f);
    env.setRelease(0.01f);   // 480 samples

    env.noteOn();
    for (int i = 0; i < 100; ++i)
        (void) env.processSample();

    env.noteOff();

    const std::vector<float> samples = render(env, 600);

    const int firstAtZero = firstSampleAtOrBelow(samples, 0.0f);
    REQUIRE(firstAtZero >= 0);
    REQUIRE(firstAtZero <= 485);
    REQUIRE(samples.back() == 0.0f);
    REQUIRE_FALSE(env.isActive());
}

TEST_CASE("AmpEnvelope: isActive reflects stage transitions", "[dsp][ampenv]")
{
    AmpEnvelope env;
    env.prepare(kSampleRate);
    env.setAttack(0.01f);
    env.setDecay(0.01f);
    env.setSustain(0.5f);
    env.setRelease(0.01f);

    REQUIRE_FALSE(env.isActive());

    env.noteOn();
    REQUIRE(env.isActive());          // attack

    for (int i = 0; i < 500; ++i)
        (void) env.processSample();
    REQUIRE(env.isActive());          // past attack (into decay/sustain)

    for (int i = 0; i < 500; ++i)
        (void) env.processSample();
    REQUIRE(env.isActive());          // sustain

    env.noteOff();
    REQUIRE(env.isActive());          // release

    for (int i = 0; i < 500; ++i)
        (void) env.processSample();
    REQUIRE_FALSE(env.isActive());    // back to idle
}

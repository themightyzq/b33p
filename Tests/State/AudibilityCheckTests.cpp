#include <catch2/catch_test_macros.hpp>

#include "Core/ParameterIDs.h"
#include "State/AudibilityCheck.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace
{
    constexpr double kSampleRate            = 48000.0;
    constexpr double kAuditionDurationSecs  = 0.5;

    // Writes a real-world parameter value (Hz, seconds, choice index)
    // through the parameter's normalised range, so tests can pin the
    // synth to a known configuration with the same API the audibility
    // fallback uses.
    void writeRealValue(juce::AudioProcessorValueTreeState& apvts,
                        const juce::String& id,
                        float realValue)
    {
        auto* p = apvts.getParameter(id);
        REQUIRE(p != nullptr);
        const float norm = p->getNormalisableRange().convertTo0to1(realValue);
        p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, norm));
    }
}

TEST_CASE("AudibilityCheck::isNearSilent threshold sits at ~ -40 dBFS",
          "[state][audibility]")
{
    using B33p::AudibilityCheck::isNearSilent;
    using B33p::AudibilityCheck::kAudibilityFloorLinear;

    REQUIRE(isNearSilent(0.0f));
    REQUIRE(isNearSilent(kAudibilityFloorLinear * 0.5f));
    REQUIRE_FALSE(isNearSilent(kAudibilityFloorLinear));
    REQUIRE_FALSE(isNearSilent(0.5f));
    REQUIRE_FALSE(isNearSilent(1.0f));
}

TEST_CASE("AudibilityCheck::measureLanePeakOffline reports the default patch as audible",
          "[state][audibility]")
{
    // A fresh processor uses the documented "audible by default"
    // parameter defaults; the offline render should comfortably clear
    // the audibility floor.
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();

    const float peak = B33p::AudibilityCheck::measureLanePeakOffline(
        apvts, /*lane=*/0, kSampleRate, kAuditionDurationSecs);

    REQUIRE(peak > B33p::AudibilityCheck::kAudibilityFloorLinear);
}

TEST_CASE("AudibilityCheck::measureLanePeakOffline detects a Mod-FX-silenced patch",
          "[state][audibility]")
{
    // Mod FX = Delay (index 3) with delay time at max (2 s) and mix
    // fully wet → the wet output is silent for the entire 0.5 s
    // audition (the delay line hasn't yet returned the dry signal),
    // which is exactly the kind of combinational silence the rule
    // exists to catch.
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();

    writeRealValue(apvts, B33p::ParameterIDs::modEffectType(0),
                   /*Delay choice index*/ 3.0f);
    writeRealValue(apvts, B33p::ParameterIDs::modEffectParam1(0), 1.0f);
    writeRealValue(apvts, B33p::ParameterIDs::modEffectMix(0),    1.0f);

    const float peak = B33p::AudibilityCheck::measureLanePeakOffline(
        apvts, /*lane=*/0, kSampleRate, kAuditionDurationSecs);

    REQUIRE(peak < B33p::AudibilityCheck::kAudibilityFloorLinear);
}

TEST_CASE("AudibilityCheck::forceLaneToAudibleFallback recovers a silent lane",
          "[state][audibility]")
{
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();

    // Silence the lane via the same Mod-FX trick the previous test uses.
    writeRealValue(apvts, B33p::ParameterIDs::modEffectType(0),    3.0f);
    writeRealValue(apvts, B33p::ParameterIDs::modEffectParam1(0),  1.0f);
    writeRealValue(apvts, B33p::ParameterIDs::modEffectMix(0),     1.0f);
    REQUIRE(B33p::AudibilityCheck::measureLanePeakOffline(
                apvts, 0, kSampleRate, kAuditionDurationSecs)
            < B33p::AudibilityCheck::kAudibilityFloorLinear);

    B33p::AudibilityCheck::forceLaneToAudibleFallback(apvts, 0);

    const float peak = B33p::AudibilityCheck::measureLanePeakOffline(
        apvts, 0, kSampleRate, kAuditionDurationSecs);
    REQUIRE(peak > B33p::AudibilityCheck::kAudibilityFloorLinear);
}

TEST_CASE("ensureLaneAudibleAfterRandomize always leaves the lane audible across seeds",
          "[state][audibility][rule]")
{
    // The headline rule. Across many random seeds, roll every unlocked
    // parameter on lane 0, then run the audibility guard. The guard
    // either re-rolls level-critical params back to an audible patch or
    // falls back to safe values — either way the post-render peak must
    // clear the floor.
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();
    auto& randomizer = processor.getRandomizer();

    constexpr int kSeeds = 64;
    for (int seed = 0; seed < kSeeds; ++seed)
    {
        juce::Random rng(seed);
        randomizer.rollMany(B33p::ParameterIDs::allForLane(0), rng,
                            "Test roll seed " + juce::String(seed));
        processor.ensureLaneAudibleAfterRandomize(0, rng);

        const float peak = B33p::AudibilityCheck::measureLanePeakOffline(
            apvts, 0, kSampleRate, kAuditionDurationSecs);
        INFO("seed=" << seed << "  peak=" << peak);
        REQUIRE(peak > B33p::AudibilityCheck::kAudibilityFloorLinear);
    }
}

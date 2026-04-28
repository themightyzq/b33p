#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Core/ParameterIDs.h"
#include "State/B33pProcessor.h"
#include "State/ParameterRandomizer.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <set>

using Catch::Approx;

TEST_CASE("ParameterRandomizer: rollOne on an unlocked parameter changes its value", "[state][randomizer]")
{
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();
    B33p::ParameterRandomizer randomizer(apvts);
    juce::Random rng(42);

    const juce::String id     = B33p::ParameterIDs::filterCutoffHz(0);
    const float        before = apvts.getRawParameterValue(id)->load();

    REQUIRE(randomizer.rollOne(id, rng));
    REQUIRE(apvts.getRawParameterValue(id)->load() != Approx(before));
}

TEST_CASE("ParameterRandomizer: rollOne on a locked parameter is a no-op", "[state][randomizer]")
{
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();
    B33p::ParameterRandomizer randomizer(apvts);
    juce::Random rng(42);

    const juce::String id     = B33p::ParameterIDs::filterCutoffHz(0);
    const float        before = apvts.getRawParameterValue(id)->load();

    randomizer.setLocked(id, true);
    REQUIRE(randomizer.isLocked(id));

    REQUIRE_FALSE(randomizer.rollOne(id, rng));
    REQUIRE(apvts.getRawParameterValue(id)->load() == before);
}

TEST_CASE("ParameterRandomizer: rolled amp_release stays at or below 100 ms",
          "[state][randomizer]")
{
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();
    B33p::ParameterRandomizer randomizer(apvts);
    juce::Random rng(42);

    const juce::String id = B33p::ParameterIDs::ampRelease(0);
    for (int i = 0; i < 100; ++i)
    {
        randomizer.rollOne(id, rng);
        const float v = apvts.getRawParameterValue(id)->load();
        INFO("iteration " << i << " value " << v);
        REQUIRE(v <= 0.1f + 1e-4f);
    }
}

TEST_CASE("ParameterRandomizer: rollOne on an unknown parameter ID safely returns false", "[state][randomizer]")
{
    B33p::B33pProcessor processor;
    B33p::ParameterRandomizer randomizer(processor.getApvts());
    juce::Random rng(42);

    REQUIRE_FALSE(randomizer.rollOne("nonexistent_param", rng));
}

TEST_CASE("ParameterRandomizer: rolled values stay inside the parameter's range", "[state][randomizer]")
{
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();
    B33p::ParameterRandomizer randomizer(apvts);
    juce::Random rng(42);

    const juce::String id    = B33p::ParameterIDs::filterCutoffHz(0);
    auto*              param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(id));
    REQUIRE(param != nullptr);

    const auto& range = param->getNormalisableRange();

    for (int i = 0; i < 100; ++i)
    {
        randomizer.rollOne(id, rng);
        const float v = apvts.getRawParameterValue(id)->load();
        REQUIRE(v >= range.start);
        REQUIRE(v <= range.end);
    }
}

TEST_CASE("ParameterRandomizer: rollAllUnlocked skips locked parameters", "[state][randomizer]")
{
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();
    B33p::ParameterRandomizer randomizer(apvts);
    juce::Random rng(42);

    randomizer.setLocked(B33p::ParameterIDs::basePitchHz(0), true);

    const float beforeLocked   = apvts.getRawParameterValue(B33p::ParameterIDs::basePitchHz(0))->load();
    const float beforeUnlocked = apvts.getRawParameterValue(B33p::ParameterIDs::ampAttack(0))->load();

    randomizer.rollAllUnlocked(rng);

    REQUIRE(apvts.getRawParameterValue(B33p::ParameterIDs::basePitchHz(0))->load() == beforeLocked);
    REQUIRE(apvts.getRawParameterValue(B33p::ParameterIDs::ampAttack(0))->load()   != Approx(beforeUnlocked));
}

TEST_CASE("ParameterRandomizer: choice parameters roll across multiple valid indices", "[state][randomizer]")
{
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();
    B33p::ParameterRandomizer randomizer(apvts);
    juce::Random rng(42);

    auto* waveform = dynamic_cast<juce::AudioParameterChoice*>(
        apvts.getParameter(B33p::ParameterIDs::oscWaveform(0)));
    REQUIRE(waveform != nullptr);

    std::set<int> observedIndices;
    for (int i = 0; i < 100; ++i)
    {
        randomizer.rollOne(B33p::ParameterIDs::oscWaveform(0), rng);
        observedIndices.insert(waveform->getIndex());
    }

    // Five choices * 100 rolls — hitting at least three distinct
    // indices is essentially certain but leaves margin for bad luck.
    REQUIRE(observedIndices.size() >= 3);
    for (int idx : observedIndices)
    {
        REQUIRE(idx >= 0);
        REQUIRE(idx <= 4);
    }
}

TEST_CASE("ParameterRandomizer: seeded rngs produce identical outputs across instances", "[state][randomizer]")
{
    B33p::B33pProcessor proc1;
    B33p::B33pProcessor proc2;
    B33p::ParameterRandomizer r1(proc1.getApvts());
    B33p::ParameterRandomizer r2(proc2.getApvts());
    juce::Random rng1(123);
    juce::Random rng2(123);

    r1.rollAllUnlocked(rng1);
    r2.rollAllUnlocked(rng2);

    const juce::String ids[] = {
        B33p::ParameterIDs::basePitchHz(0),
        B33p::ParameterIDs::filterCutoffHz(0),
        B33p::ParameterIDs::ampAttack(0),
        B33p::ParameterIDs::voiceGain(0),
    };

    for (const auto& id : ids)
    {
        REQUIRE(proc1.getApvts().getRawParameterValue(id)->load()
                == proc2.getApvts().getRawParameterValue(id)->load());
    }
}

TEST_CASE("ParameterRandomizer: lock state persists across rolls", "[state][randomizer]")
{
    B33p::B33pProcessor processor;
    B33p::ParameterRandomizer randomizer(processor.getApvts());
    juce::Random rng(42);

    randomizer.setLocked(B33p::ParameterIDs::voiceGain(0), true);
    REQUIRE(randomizer.isLocked(B33p::ParameterIDs::voiceGain(0)));

    randomizer.rollAllUnlocked(rng);
    REQUIRE(randomizer.isLocked(B33p::ParameterIDs::voiceGain(0)));

    randomizer.setLocked(B33p::ParameterIDs::voiceGain(0), false);
    REQUIRE_FALSE(randomizer.isLocked(B33p::ParameterIDs::voiceGain(0)));
}

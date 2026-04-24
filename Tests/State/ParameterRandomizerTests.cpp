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

    const juce::String id     = B33p::ParameterIDs::filterCutoffHz;
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

    const juce::String id     = B33p::ParameterIDs::filterCutoffHz;
    const float        before = apvts.getRawParameterValue(id)->load();

    randomizer.setLocked(id, true);
    REQUIRE(randomizer.isLocked(id));

    REQUIRE_FALSE(randomizer.rollOne(id, rng));
    REQUIRE(apvts.getRawParameterValue(id)->load() == before);
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

    const juce::String id    = B33p::ParameterIDs::filterCutoffHz;
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

    randomizer.setLocked(B33p::ParameterIDs::basePitchHz, true);

    const float beforeLocked   = apvts.getRawParameterValue(B33p::ParameterIDs::basePitchHz)->load();
    const float beforeUnlocked = apvts.getRawParameterValue(B33p::ParameterIDs::ampAttack)->load();

    randomizer.rollAllUnlocked(rng);

    REQUIRE(apvts.getRawParameterValue(B33p::ParameterIDs::basePitchHz)->load() == beforeLocked);
    REQUIRE(apvts.getRawParameterValue(B33p::ParameterIDs::ampAttack)->load()   != Approx(beforeUnlocked));
}

TEST_CASE("ParameterRandomizer: choice parameters roll across multiple valid indices", "[state][randomizer]")
{
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();
    B33p::ParameterRandomizer randomizer(apvts);
    juce::Random rng(42);

    auto* waveform = dynamic_cast<juce::AudioParameterChoice*>(
        apvts.getParameter(B33p::ParameterIDs::oscWaveform));
    REQUIRE(waveform != nullptr);

    std::set<int> observedIndices;
    for (int i = 0; i < 100; ++i)
    {
        randomizer.rollOne(B33p::ParameterIDs::oscWaveform, rng);
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

    const char* const ids[] = {
        B33p::ParameterIDs::basePitchHz,
        B33p::ParameterIDs::filterCutoffHz,
        B33p::ParameterIDs::ampAttack,
        B33p::ParameterIDs::voiceGain,
    };

    for (const char* id : ids)
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

    randomizer.setLocked(B33p::ParameterIDs::voiceGain, true);
    REQUIRE(randomizer.isLocked(B33p::ParameterIDs::voiceGain));

    randomizer.rollAllUnlocked(rng);
    REQUIRE(randomizer.isLocked(B33p::ParameterIDs::voiceGain));

    randomizer.setLocked(B33p::ParameterIDs::voiceGain, false);
    REQUIRE_FALSE(randomizer.isLocked(B33p::ParameterIDs::voiceGain));
}

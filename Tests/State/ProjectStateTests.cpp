#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Core/ParameterIDs.h"
#include "State/B33pProcessor.h"
#include "State/ProjectState.h"

using B33p::B33pProcessor;
using Catch::Approx;

namespace
{
    // Reusable helper that mutates a processor away from its
    // defaults so save/load actually has something to round-trip.
    void mutateProcessor(B33pProcessor& processor)
    {
        auto& apvts = processor.getApvts();

        // Move several parameters off their defaults.
        if (auto* p = apvts.getParameter(B33p::ParameterIDs::basePitchHz))
            p->setValueNotifyingHost(0.25f);   // some non-default normalised value
        if (auto* p = apvts.getParameter(B33p::ParameterIDs::ampAttack))
            p->setValueNotifyingHost(0.5f);
        if (auto* p = apvts.getParameter(B33p::ParameterIDs::voiceGain))
            p->setValueNotifyingHost(0.7f);

        // Custom pitch curve.
        processor.setPitchCurve({
            { 0.0f,  -7.0f },
            { 0.5f,   3.0f },
            { 1.0f, -12.0f }
        });

        // Pattern with non-default length, looping off, a couple of
        // events spread across multiple lanes.
        auto& pattern = processor.getPattern();
        pattern.setLengthSeconds(3.0);
        pattern.addEvent(0, { 0.5, 0.25, 0.0f });
        pattern.addEvent(2, { 1.5, 0.10, 7.0f });
        processor.setLooping(false);

        // Lock a couple of parameters.
        auto& randomizer = processor.getRandomizer();
        randomizer.setLocked(B33p::ParameterIDs::filterCutoffHz, true);
        randomizer.setLocked(B33p::ParameterIDs::distortionDrive, true);
    }
}

TEST_CASE("ProjectState: save returns a B33P-typed tree at the current version",
          "[state][project]")
{
    B33pProcessor processor;
    const auto tree = B33p::ProjectState::save(processor);

    REQUIRE(tree.hasType(juce::Identifier("B33P")));
    REQUIRE(static_cast<int>(tree.getProperty("version", 0))
            == B33p::ProjectState::kCurrentVersion);
}

TEST_CASE("ProjectState: round-trip preserves APVTS values", "[state][project]")
{
    B33pProcessor original;
    mutateProcessor(original);

    const auto tree = B33p::ProjectState::save(original);

    B33pProcessor restored;
    REQUIRE(B33p::ProjectState::load(restored, tree));

    auto& a = original.getApvts();
    auto& b = restored.getApvts();

    const char* const ids[] = {
        B33p::ParameterIDs::basePitchHz,
        B33p::ParameterIDs::ampAttack,
        B33p::ParameterIDs::voiceGain,
        B33p::ParameterIDs::filterCutoffHz,
    };
    for (const char* id : ids)
        REQUIRE(a.getRawParameterValue(id)->load()
                == Approx(b.getRawParameterValue(id)->load()));
}

TEST_CASE("ProjectState: round-trip preserves pitch curve", "[state][project]")
{
    B33pProcessor original;
    mutateProcessor(original);

    const auto tree = B33p::ProjectState::save(original);

    B33pProcessor restored;
    REQUIRE(B33p::ProjectState::load(restored, tree));

    const auto a = original.getPitchCurveCopy();
    const auto b = restored.getPitchCurveCopy();
    REQUIRE(a.size() == b.size());
    for (size_t i = 0; i < a.size(); ++i)
    {
        REQUIRE(a[i].normalizedTime == Approx(b[i].normalizedTime));
        REQUIRE(a[i].semitones      == Approx(b[i].semitones));
    }
}

TEST_CASE("ProjectState: round-trip preserves pattern length, loop and events",
          "[state][project]")
{
    B33pProcessor original;
    mutateProcessor(original);

    const auto tree = B33p::ProjectState::save(original);

    B33pProcessor restored;
    REQUIRE(B33p::ProjectState::load(restored, tree));

    REQUIRE(restored.getPattern().getLengthSeconds()
            == Approx(original.getPattern().getLengthSeconds()));
    REQUIRE(restored.getLooping() == original.getLooping());

    for (int lane = 0; lane < B33p::Pattern::kNumLanes; ++lane)
    {
        const auto& a = original.getPattern().getEvents(lane);
        const auto& b = restored.getPattern().getEvents(lane);
        REQUIRE(a.size() == b.size());
        for (size_t i = 0; i < a.size(); ++i)
        {
            REQUIRE(a[i].startSeconds         == Approx(b[i].startSeconds));
            REQUIRE(a[i].durationSeconds      == Approx(b[i].durationSeconds));
            REQUIRE(a[i].pitchOffsetSemitones == Approx(b[i].pitchOffsetSemitones));
        }
    }
}

TEST_CASE("ProjectState: round-trip preserves locks", "[state][project]")
{
    B33pProcessor original;
    mutateProcessor(original);

    const auto tree = B33p::ProjectState::save(original);

    B33pProcessor restored;
    REQUIRE(B33p::ProjectState::load(restored, tree));

    REQUIRE(restored.getRandomizer().isLocked(B33p::ParameterIDs::filterCutoffHz));
    REQUIRE(restored.getRandomizer().isLocked(B33p::ParameterIDs::distortionDrive));
    REQUIRE_FALSE(restored.getRandomizer().isLocked(B33p::ParameterIDs::basePitchHz));
}

TEST_CASE("ProjectState: load rejects a non-B33P root", "[state][project]")
{
    B33pProcessor processor;

    juce::ValueTree wrongRoot { "NOT_B33P" };
    REQUIRE_FALSE(B33p::ProjectState::load(processor, wrongRoot));
}

TEST_CASE("ProjectState: load rejects a future version", "[state][project]")
{
    B33pProcessor processor;

    juce::ValueTree future { "B33P" };
    future.setProperty("version", B33p::ProjectState::kCurrentVersion + 1, nullptr);

    REQUIRE_FALSE(B33p::ProjectState::load(processor, future));
}

TEST_CASE("ProjectState: load rejects a missing version", "[state][project]")
{
    B33pProcessor processor;

    juce::ValueTree noVersion { "B33P" };
    REQUIRE_FALSE(B33p::ProjectState::load(processor, noVersion));
}

TEST_CASE("ProjectState: XML round-trip survives the disk-format pass",
          "[state][project]")
{
    B33pProcessor original;
    mutateProcessor(original);

    const auto tree    = B33p::ProjectState::save(original);
    const auto xml     = B33p::ProjectState::toXmlString(tree);
    const auto reparsed = B33p::ProjectState::fromXmlString(xml);

    REQUIRE(reparsed.isValid());

    B33pProcessor restored;
    REQUIRE(B33p::ProjectState::load(restored, reparsed));

    // One canary value to confirm reparse made it all the way through.
    REQUIRE(restored.getPattern().getLengthSeconds()
            == Approx(original.getPattern().getLengthSeconds()));
    REQUIRE(restored.getLooping() == original.getLooping());
}

TEST_CASE("ProjectState: getStateInformation/setStateInformation round-trip",
          "[state][project]")
{
    B33pProcessor original;
    mutateProcessor(original);

    juce::MemoryBlock memory;
    original.getStateInformation(memory);
    REQUIRE(memory.getSize() > 0);

    B33pProcessor restored;
    restored.setStateInformation(memory.getData(), static_cast<int>(memory.getSize()));

    REQUIRE(restored.getPattern().getLengthSeconds()
            == Approx(original.getPattern().getLengthSeconds()));
    REQUIRE(restored.getLooping() == original.getLooping());
    REQUIRE(restored.getRandomizer().isLocked(B33p::ParameterIDs::filterCutoffHz));
}

TEST_CASE("ProjectState: writeToFile / readFromFile round-trip via temp file",
          "[state][project]")
{
    B33pProcessor original;
    mutateProcessor(original);

    const auto tmp = juce::File::createTempFile(".beep");
    REQUIRE(B33p::ProjectState::writeToFile(original, tmp));
    REQUIRE(tmp.existsAsFile());
    REQUIRE(tmp.getSize() > 0);

    B33pProcessor restored;
    REQUIRE(B33p::ProjectState::readFromFile(restored, tmp));

    REQUIRE(restored.getPattern().getLengthSeconds()
            == Approx(original.getPattern().getLengthSeconds()));
    REQUIRE(restored.getLooping() == original.getLooping());
    REQUIRE(restored.getRandomizer().isLocked(B33p::ParameterIDs::filterCutoffHz));

    tmp.deleteFile();
}

TEST_CASE("ProjectState: readFromFile rejects a missing file", "[state][project]")
{
    B33pProcessor processor;

    const juce::File missing { "/nonexistent_b33p_dir/nope.beep" };
    REQUIRE_FALSE(B33p::ProjectState::readFromFile(processor, missing));
}

TEST_CASE("ProjectState: readFromFile rejects malformed XML", "[state][project]")
{
    B33pProcessor processor;

    const auto tmp = juce::File::createTempFile(".beep");
    REQUIRE(tmp.replaceWithText("this is not <valid> xml{{}"));
    REQUIRE_FALSE(B33p::ProjectState::readFromFile(processor, tmp));

    tmp.deleteFile();
}

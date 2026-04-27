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
        if (auto* p = apvts.getParameter(B33p::ParameterIDs::basePitchHz(0)))
            p->setValueNotifyingHost(0.25f);   // some non-default normalised value
        if (auto* p = apvts.getParameter(B33p::ParameterIDs::ampAttack(0)))
            p->setValueNotifyingHost(0.5f);
        if (auto* p = apvts.getParameter(B33p::ParameterIDs::voiceGain(0)))
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
        pattern.addEvent(0, { 0.5, 0.25, 0.0f, 1.0f });
        pattern.addEvent(2, { 1.5, 0.10, 7.0f, 0.6f });   // non-default velocity
        pattern.setLaneName(0, "Body");                    // non-default name
        pattern.setLaneName(2, "Tail");
        pattern.setLaneMuted(1, true);                     // non-default mute
        processor.setLooping(false);

        // Lock a couple of parameters.
        auto& randomizer = processor.getRandomizer();
        randomizer.setLocked(B33p::ParameterIDs::filterCutoffHz(0), true);
        randomizer.setLocked(B33p::ParameterIDs::distortionDrive(0), true);
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

    const juce::String ids[] = {
        B33p::ParameterIDs::basePitchHz(0),
        B33p::ParameterIDs::ampAttack(0),
        B33p::ParameterIDs::voiceGain(0),
        B33p::ParameterIDs::filterCutoffHz(0),
    };
    for (const auto& id : ids)
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
            REQUIRE(a[i].velocity             == Approx(b[i].velocity));
        }

        REQUIRE(original.getPattern().getLaneName(lane)
                == restored.getPattern().getLaneName(lane));
        REQUIRE(original.getPattern().isLaneMuted(lane)
                == restored.getPattern().isLaneMuted(lane));
    }
}

TEST_CASE("ProjectState: v1 file fans the single voice's APVTS params out to all 4 lanes",
          "[state][project]")
{
    // Hand-rolled v1 tree: one APVTS PARAM per param ID (no lane prefix),
    // version=1. After load + migrate, all four lanes should carry the
    // same value the v1 file specified.
    juce::ValueTree root { "B33P" };
    root.setProperty("version", 1, nullptr);

    juce::ValueTree params { "PARAMETERS" };
    juce::ValueTree apvts  { "B33pParameters" };

    auto addParam = [&apvts](const juce::String& id, double value)
    {
        juce::ValueTree p { "PARAM" };
        p.setProperty("id",    id,    nullptr);
        p.setProperty("value", value, nullptr);
        apvts.appendChild(p, nullptr);
    };
    addParam(B33p::ParameterIDs::v1::basePitchHz,    880.0);
    addParam(B33p::ParameterIDs::v1::voiceGain,      0.42);
    addParam(B33p::ParameterIDs::v1::filterCutoffHz, 1234.0);

    params.appendChild(apvts, nullptr);
    root.appendChild(params, nullptr);

    B33pProcessor processor;
    REQUIRE(B33p::ProjectState::load(processor, root));

    auto& a = processor.getApvts();
    for (int lane = 0; lane < B33p::Pattern::kNumLanes; ++lane)
    {
        REQUIRE(a.getRawParameterValue(B33p::ParameterIDs::basePitchHz(lane))->load()
                == Approx(880.0f).margin(1.0f));
        REQUIRE(a.getRawParameterValue(B33p::ParameterIDs::voiceGain(lane))->load()
                == Approx(0.42f).margin(0.001f));
        REQUIRE(a.getRawParameterValue(B33p::ParameterIDs::filterCutoffHz(lane))->load()
                == Approx(1234.0f).margin(1.0f));
    }
}

TEST_CASE("ProjectState: events from a v1 file (no velocity property) load with velocity 1.0",
          "[state][project]")
{
    // Synthesize a v1-style tree by hand — same structure ProjectState
    // generates, but with the EVENT lacking the new "velocity" attribute.
    juce::ValueTree root { "B33P" };
    root.setProperty("version", B33p::ProjectState::kCurrentVersion, nullptr);

    juce::ValueTree params { "PARAMETERS" };
    root.appendChild(params, nullptr);

    juce::ValueTree pattern { "PATTERN" };
    pattern.setProperty("length_seconds", 2.0, nullptr);
    pattern.setProperty("looping",        true, nullptr);

    juce::ValueTree lane { "LANE" };
    lane.setProperty("index", 0, nullptr);

    juce::ValueTree event { "EVENT" };
    event.setProperty("start_seconds",          0.5, nullptr);
    event.setProperty("duration_seconds",       0.2, nullptr);
    event.setProperty("pitch_offset_semitones", 0.0, nullptr);
    // No "velocity" property — this is what a pre-Phase-8 file looks like.
    lane.appendChild(event, nullptr);
    pattern.appendChild(lane, nullptr);
    root.appendChild(pattern, nullptr);

    B33pProcessor restored;
    REQUIRE(B33p::ProjectState::load(restored, root));

    const auto& events = restored.getPattern().getEvents(0);
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].velocity == Approx(1.0f));
}

TEST_CASE("ProjectState: round-trip preserves locks", "[state][project]")
{
    B33pProcessor original;
    mutateProcessor(original);

    const auto tree = B33p::ProjectState::save(original);

    B33pProcessor restored;
    REQUIRE(B33p::ProjectState::load(restored, tree));

    REQUIRE(restored.getRandomizer().isLocked(B33p::ParameterIDs::filterCutoffHz(0)));
    REQUIRE(restored.getRandomizer().isLocked(B33p::ParameterIDs::distortionDrive(0)));
    REQUIRE_FALSE(restored.getRandomizer().isLocked(B33p::ParameterIDs::basePitchHz(0)));
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
    REQUIRE(restored.getRandomizer().isLocked(B33p::ParameterIDs::filterCutoffHz(0)));
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
    REQUIRE(restored.getRandomizer().isLocked(B33p::ParameterIDs::filterCutoffHz(0)));

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

// ---------------------------------------------------------------
// Dirty-state machine
// ---------------------------------------------------------------

TEST_CASE("Dirty: a fresh processor starts clean", "[state][dirty]")
{
    B33pProcessor processor;
    REQUIRE_FALSE(processor.isDirty());
}

TEST_CASE("Dirty: setting an APVTS parameter marks dirty", "[state][dirty]")
{
    B33pProcessor processor;
    REQUIRE_FALSE(processor.isDirty());

    if (auto* p = processor.getApvts().getParameter(B33p::ParameterIDs::voiceGain(0)))
        p->setValueNotifyingHost(0.42f);

    REQUIRE(processor.isDirty());
}

TEST_CASE("Dirty: setPitchCurve marks dirty", "[state][dirty]")
{
    B33pProcessor processor;
    REQUIRE_FALSE(processor.isDirty());

    processor.setPitchCurve({ { 0.0f, 0.0f }, { 1.0f, 5.0f } });
    REQUIRE(processor.isDirty());
}

TEST_CASE("Dirty: setLooping marks dirty only on actual change", "[state][dirty]")
{
    B33pProcessor processor;
    const bool initial = processor.getLooping();

    // Toggling from initial state must mark dirty.
    processor.setLooping(! initial);
    REQUIRE(processor.isDirty());

    processor.markClean();
    REQUIRE_FALSE(processor.isDirty());

    // Setting to the same value is a no-op — no dirty flag.
    processor.setLooping(! initial);
    REQUIRE_FALSE(processor.isDirty());
}

TEST_CASE("Dirty: markClean clears the flag", "[state][dirty]")
{
    B33pProcessor processor;
    processor.markDirty();
    REQUIRE(processor.isDirty());

    processor.markClean();
    REQUIRE_FALSE(processor.isDirty());
}

TEST_CASE("Dirty: ProjectState::load leaves the processor clean", "[state][dirty]")
{
    B33pProcessor original;
    mutateProcessor(original);
    REQUIRE(original.isDirty());

    const auto tree = B33p::ProjectState::save(original);

    B33pProcessor target;
    target.markDirty();   // simulate user having edits before opening
    REQUIRE(target.isDirty());

    REQUIRE(B33p::ProjectState::load(target, tree));
    REQUIRE_FALSE(target.isDirty());
}

TEST_CASE("Dirty: writeToFile + markClean produces a clean processor",
          "[state][dirty]")
{
    B33pProcessor processor;
    mutateProcessor(processor);
    REQUIRE(processor.isDirty());

    const auto tmp = juce::File::createTempFile(".beep");
    REQUIRE(B33p::ProjectState::writeToFile(processor, tmp));

    // writeToFile alone does not flip dirty (it doesn't know about
    // a "current file" concept). The caller — ProjectFileManager —
    // marks clean after a successful write. Simulate that here.
    processor.markClean();
    REQUIRE_FALSE(processor.isDirty());

    tmp.deleteFile();
}

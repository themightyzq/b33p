#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/AmpEnvelope.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

using B33p::AmpEnvelope;
using B33p::B33pProcessor;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate    = 48000.0;
    constexpr int    kBlockSize     = 256;

    void runOneBlock(B33pProcessor& processor)
    {
        juce::AudioBuffer<float> buf(2, kBlockSize);
        juce::MidiBuffer midi;
        processor.processBlock(buf, midi);
    }
}

TEST_CASE("B33pProcessor: amp-env snapshot publishes Idle on a fresh processor", "[state][p31]")
{
    // Pre-prepare, the packed atomic is its constructed default (all zeros).
    // After prepare the audio thread hasn't run yet, so the snapshot stays
    // at Idle / 0. Both code paths must decode as Idle (stage 0).
    B33pProcessor processor;

    auto snap = processor.getSelectedLaneAmpEnvSnapshot();
    REQUIRE(snap.stageInt == static_cast<int>(AmpEnvelope::Stage::Idle));
    REQUIRE(snap.elapsedSec == Approx(0.0f).margin(1e-6f));

    processor.prepareToPlay(kSampleRate, kBlockSize);
    snap = processor.getSelectedLaneAmpEnvSnapshot();
    REQUIRE(snap.stageInt == static_cast<int>(AmpEnvelope::Stage::Idle));
    REQUIRE(snap.elapsedSec == Approx(0.0f).margin(1e-6f));
}

TEST_CASE("B33pProcessor: amp-env snapshot reflects an active note after audition", "[state][p31]")
{
    // After triggerAudition + one processBlock, the selected lane's amp
    // envelope is non-Idle and its elapsed-in-stage matches the time the
    // audio thread spent in that stage (within one block's worth of
    // tolerance — block boundary plus a sample or two of stage transition).
    B33pProcessor processor;
    processor.prepareToPlay(kSampleRate, kBlockSize);
    processor.triggerAudition();
    runOneBlock(processor);

    const auto snap = processor.getSelectedLaneAmpEnvSnapshot();
    REQUIRE(snap.stageInt != static_cast<int>(AmpEnvelope::Stage::Idle));
    REQUIRE(snap.elapsedSec >= 0.0f);
    // One block at 48 kHz is ~5.3 ms. The envelope's elapsed should be
    // somewhere in [0, 1 block worth of time + a stage-transition tolerance].
    REQUIRE(snap.elapsedSec < 0.020f);
}

TEST_CASE("B33pProcessor: amp-env snapshot encodes stage + elapsed atomically (decode round-trip)",
          "[state][p31]")
{
    // The bug the packed-atomic publication was written to fix was a
    // torn read between two separate atomics. This test exercises the
    // round-trip: trigger audition, run many blocks, and at every block
    // boundary verify the snapshot decodes to a consistent (stage,
    // elapsed) pair — stage in 0..4, elapsed >= 0 and reasonable for
    // the stage.
    B33pProcessor processor;
    processor.prepareToPlay(kSampleRate, kBlockSize);
    processor.triggerAudition();

    constexpr int kBlocksToRun = 64;
    for (int i = 0; i < kBlocksToRun; ++i)
    {
        runOneBlock(processor);
        const auto snap = processor.getSelectedLaneAmpEnvSnapshot();
        REQUIRE(snap.stageInt >= static_cast<int>(AmpEnvelope::Stage::Idle));
        REQUIRE(snap.stageInt <= static_cast<int>(AmpEnvelope::Stage::Release));
        REQUIRE(snap.elapsedSec >= 0.0f);
        // No stage runs forever — at the slider caps, the longest is
        // a 5 s release. We've only run 64 blocks (~340 ms) so elapsed
        // must be well under any stage's max.
        REQUIRE(snap.elapsedSec < 1.0f);
    }
}

TEST_CASE("B33pProcessor: amp-env snapshot returns to Idle after the note tail", "[state][p31]")
{
    // Audition runs for kAuditionDurationSeconds (0.5 s) then noteOff;
    // the amp env runs through Release and back to Idle. Run plenty of
    // blocks past the audition + release (default release = 0.1 s), then
    // confirm the snapshot has settled to Idle.
    B33pProcessor processor;
    processor.prepareToPlay(kSampleRate, kBlockSize);
    processor.triggerAudition();

    // Audition 0.5 s + release ~0.1 s + safety margin = ~0.8 s.
    // 0.8 s / (kBlockSize / kSampleRate) = 0.8 / 0.00533 = ~150 blocks.
    for (int i = 0; i < 200; ++i)
        runOneBlock(processor);

    const auto snap = processor.getSelectedLaneAmpEnvSnapshot();
    REQUIRE(snap.stageInt == static_cast<int>(AmpEnvelope::Stage::Idle));
    REQUIRE(snap.elapsedSec == Approx(0.0f).margin(1e-6f));
}

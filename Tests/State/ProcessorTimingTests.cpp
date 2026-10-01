#include <catch2/catch_test_macros.hpp>

#include "Core/ParameterIDs.h"
#include "DSP/ModulationMatrix.h"
#include "DSP/Oscillator.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

using B33p::B33pProcessor;
using B33p::Event;
using B33p::ModDestination;

namespace IDs = B33p::ParameterIDs;

// Sample-accurate timing of MIDI notes and of per-event overrides
// (B33pProcessor::processBlock). Both used to land at block granularity:
// MIDI was applied at the start of the block whatever its sample position,
// and a pattern event's per-event overrides reached the batched effects
// tail (bitcrush / distortion / Mod FX / gain) from the start of the block
// the event fell in, up to 2047 samples early.

namespace
{
    constexpr double kSampleRate = 48000.0;

    void setReal(B33pProcessor& p, const juce::String& id, float realValue)
    {
        auto* param = p.getApvts().getParameter(id);
        REQUIRE(param != nullptr);
        const float norm = param->getNormalisableRange().convertTo0to1(realValue);
        param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, norm));
    }

    // One processBlock call per entry of `blockSizes`; `midiAt` holds MIDI
    // messages with an ABSOLUTE sample position, each delivered in the block
    // that contains it at its offset within that block.
    struct TimedMidi
    {
        juce::MidiMessage message;
        int               absoluteSample;
    };

    std::vector<float> render(B33pProcessor& p,
                              const std::vector<int>& blockSizes,
                              const std::vector<TimedMidi>& midiAt)
    {
        std::vector<float> out;
        int start = 0;
        for (int bs : blockSizes)
        {
            juce::AudioBuffer<float> block(2, bs);
            block.clear();
            juce::MidiBuffer midi;
            for (const auto& m : midiAt)
                if (m.absoluteSample >= start && m.absoluteSample < start + bs)
                    midi.addEvent(m.message, m.absoluteSample - start);
            p.processBlock(block, midi);
            for (int i = 0; i < bs; ++i)
                out.push_back(block.getSample(0, i));
            start += bs;
        }
        return out;
    }

    std::vector<int> uniformBlocks(int blockSize, int totalSamples)
    {
        std::vector<int> sizes;
        for (int done = 0; done < totalSamples; done += blockSize)
            sizes.push_back(std::min(blockSize, totalSamples - done));
        return sizes;
    }

    int firstDifference(const std::vector<float>& a, const std::vector<float>& b)
    {
        REQUIRE(a.size() == b.size());
        for (size_t i = 0; i < a.size(); ++i)
            if (! juce::exactlyEqual(a[i], b[i]))   // bit-exact comparison is the point
                return static_cast<int>(i);
        return -1;
    }

    int firstNonZero(const std::vector<float>& a)
    {
        for (size_t i = 0; i < a.size(); ++i)
            if (! juce::exactlyEqual(a[i], 0.0f))
                return static_cast<int>(i);
        return -1;
    }
}

TEST_CASE("B33pProcessor: a MIDI note-on lands on its own sample, not the block start",
          "[state][timing][midi]")
{
    constexpr int kNoteOnSample = 300;
    const std::vector<TimedMidi> midi {
        { juce::MidiMessage::noteOn(1, 64, 0.9f), kNoteOnSample },
    };

    // One 512-sample block with the note-on stamped at sample 300.
    auto oneBlock = std::make_unique<B33pProcessor>();
    oneBlock->prepareToPlay(kSampleRate, 512);
    const int latency = oneBlock->getLatencySamples();
    const auto a = render(*oneBlock, { 512 }, midi);

    // Silence before the note-on sample, sound shortly after it (the
    // oversampling filters are IIR, so the first non-zero output appears
    // at or just after the note-on, within the reported latency).
    const int onset = firstNonZero(a);
    REQUIRE(onset >= kNoteOnSample);
    REQUIRE(onset <= kNoteOnSample + latency + 8);

    // Reference: the same note delivered at offset 0 of a block that
    // starts at sample 300. Sample-accurate MIDI makes the two identical.
    auto split = std::make_unique<B33pProcessor>();
    split->prepareToPlay(kSampleRate, 512);
    const auto b = render(*split, { 300, 212 }, midi);
    REQUIRE(firstDifference(a, b) == -1);
}

TEST_CASE("B33pProcessor: MIDI note-on and note-off timing is block-size invariant",
          "[state][timing][midi]")
{
    const std::vector<TimedMidi> midi {
        { juce::MidiMessage::noteOn (1, 60, 0.8f), 300  },
        { juce::MidiMessage::noteOn (1, 67, 0.6f), 1111 },
        { juce::MidiMessage::noteOff(1, 60),       2077 },
        { juce::MidiMessage::noteOff(1, 67),       3333 },
    };
    constexpr int kTotal = 8192;

    auto big = std::make_unique<B33pProcessor>();
    big->prepareToPlay(kSampleRate, 2048);
    const auto a = render(*big, uniformBlocks(2048, kTotal), midi);

    auto small = std::make_unique<B33pProcessor>();
    small->prepareToPlay(kSampleRate, 2048);
    const auto b = render(*small, uniformBlocks(64, kTotal), midi);

    REQUIRE(firstNonZero(a) >= 300);
    REQUIRE(firstDifference(a, b) == -1);
}

namespace
{
    // Lane 0: a held sine, then a second event at kSecondEventSample that
    // retriggers it, optionally carrying per-event overrides on two
    // destinations that are applied in the batched effects tail (voice gain
    // and distortion drive).
    constexpr int kSecondEventSample = 1300;

    std::unique_ptr<B33pProcessor> makeOverrideProcessor(bool withOverrides, int blockSize)
    {
        auto p = std::make_unique<B33pProcessor>();
        setReal(*p, IDs::oscWaveform(0),
                static_cast<float>(B33p::Oscillator::Waveform::Sine));
        setReal(*p, IDs::basePitchHz(0),     220.0f);
        setReal(*p, IDs::ampAttack(0),       0.0f);
        setReal(*p, IDs::ampSustain(0),      1.0f);
        setReal(*p, IDs::voiceGain(0),       0.5f);
        setReal(*p, IDs::distortionDrive(0), 1.0f);

        auto& pattern = p->getPattern();
        pattern.setLengthSeconds(1.0);
        Event first;
        first.startSeconds    = 0.0;
        first.durationSeconds = 0.5;
        pattern.addEvent(0, first);

        Event second;
        second.startSeconds    = static_cast<double>(kSecondEventSample) / kSampleRate;
        second.durationSeconds = 0.2;
        if (withOverrides)
        {
            second.overrides[0] = { ModDestination::VoiceGain,       0.05f };
            second.overrides[1] = { ModDestination::DistortionDrive, 0.9f  };
        }
        pattern.addEvent(0, second);

        p->setLooping(false);
        p->prepareToPlay(kSampleRate, blockSize);
        p->startPlayback();
        return p;
    }
}

TEST_CASE("B33pProcessor: per-event overrides on the effects tail start at the event's sample",
          "[state][timing][overrides]")
{
    constexpr int kTotal = 6144;

    for (int blockSize : { 2048, 512, 100 })
    {
        INFO("block size " << blockSize);
        auto plain = makeOverrideProcessor(false, blockSize);
        auto over  = makeOverrideProcessor(true,  blockSize);
        const auto a = render(*plain, uniformBlocks(blockSize, kTotal), {});
        const auto b = render(*over,  uniformBlocks(blockSize, kTotal), {});

        // The overrides change the sound (so the test is not vacuous) but
        // not one sample before the event that carries them. The event can
        // fire one sample late from playhead rounding, never early.
        const int diff = firstDifference(a, b);
        REQUIRE(diff >= kSecondEventSample);
        REQUIRE(diff <= kSecondEventSample + 2);
    }
}

TEST_CASE("B33pProcessor: a pattern with per-event overrides renders identically at any block size",
          "[state][timing][overrides]")
{
    constexpr int kTotal = 6144;
    auto big   = makeOverrideProcessor(true, 2048);
    auto small = makeOverrideProcessor(true, 2048);
    const auto a = render(*big,   uniformBlocks(2048, kTotal), {});
    const auto b = render(*small, uniformBlocks(64,   kTotal), {});
    REQUIRE(firstDifference(a, b) == -1);
}

// Editor-lifetime contract for the processor's message-thread
// notifications: the callback is looked up when the notification is
// delivered, so one queued before the editor cleared its callbacks (editor
// closed) calls nothing. These used to be callAsync posts of a copy of the
// callback, which still called into the destroyed editor.
TEST_CASE("B33pProcessor: queued notifications reach a live callback, and none after it is cleared",
          "[state][notifications]")
{
    auto p = std::make_unique<B33pProcessor>();
    int dirtyCalls = 0;
    int fullCalls  = 0;

    p->setOnDirtyChanged([&] { ++dirtyCalls; });
    p->setOnFullStateLoaded([&] { ++fullCalls; });
    p->markDirty();
    p->notifyFullStateLoaded();
    p->deliverPendingNotificationsForTests();
    REQUIRE(dirtyCalls == 1);
    REQUIRE(fullCalls  == 1);

    // Nothing pending: a second delivery calls nothing.
    p->deliverPendingNotificationsForTests();
    REQUIRE(dirtyCalls == 1);
    REQUIRE(fullCalls  == 1);

    // Queued, then the editor goes away before delivery.
    p->markClean();
    p->notifyFullStateLoaded();
    p->setOnDirtyChanged(nullptr);
    p->setOnFullStateLoaded(nullptr);
    p->deliverPendingNotificationsForTests();
    REQUIRE(dirtyCalls == 1);
    REQUIRE(fullCalls  == 1);
}

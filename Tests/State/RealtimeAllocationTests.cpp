#include <catch2/catch_test_macros.hpp>

#include "Core/ParameterIDs.h"
#include "DSP/ModulationEffect.h"
#include "DSP/ModulationMatrix.h"
#include "DSP/Oscillator.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <cstdlib>
#include <memory>
#include <new>

// Real-time contract (../CLAUDE.md section 4): processBlock must not
// allocate. This TU replaces the global operator new / delete for the whole
// test binary with plain malloc / free plus a per-thread counter that only
// counts while a test has switched it on, so a test can assert that a
// stretch of processBlock calls made no heap allocation at all.
//
// The aligned and nothrow forms are left to the standard library: its
// defaults forward to the forms replaced here (nothrow) or use their own
// matching allocator (aligned), so every allocation is still freed by its
// own counterpart.

// Under AddressSanitizer the runtime supplies its own operator new family;
// replacing only some forms here would pair ASan's nothrow/aligned new with
// this file's free(). The sanitizer CI job therefore skips these tests; the
// normal test jobs run them.
#if defined(__SANITIZE_ADDRESS__)
 #define B33P_ALLOC_COUNTER_DISABLED 1
#elif defined(__has_feature)
 #if __has_feature(address_sanitizer)
  #define B33P_ALLOC_COUNTER_DISABLED 1
 #endif
#endif

namespace
{
    thread_local bool countAllocations = false;
    thread_local long allocationCount  = 0;

    void* countedAllocate(std::size_t size)
    {
        if (countAllocations)
            ++allocationCount;
        if (void* p = std::malloc(size == 0 ? 1 : size))
            return p;
        throw std::bad_alloc();
    }
}

#if ! defined(B33P_ALLOC_COUNTER_DISABLED)
void* operator new  (std::size_t size)                  { return countedAllocate(size); }
void* operator new[](std::size_t size)                  { return countedAllocate(size); }
void  operator delete  (void* p) noexcept               { std::free(p); }
void  operator delete[](void* p) noexcept               { std::free(p); }
void  operator delete  (void* p, std::size_t) noexcept  { std::free(p); }
void  operator delete[](void* p, std::size_t) noexcept  { std::free(p); }
#endif

using B33p::B33pProcessor;
using B33p::Event;
using B33p::ModDestination;

namespace IDs = B33p::ParameterIDs;

namespace
{
    void setReal(B33pProcessor& p, const juce::String& id, float realValue)
    {
        auto* param = p.getApvts().getParameter(id);
        REQUIRE(param != nullptr);
        const float norm = param->getNormalisableRange().convertTo0to1(realValue);
        param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, norm));
    }

    // Counts heap allocations made on this thread inside `fn`.
    template <typename Fn>
    long allocationsDuring(Fn&& fn)
    {
        allocationCount  = 0;
        countAllocations = true;
        fn();
        countAllocations = false;
        return allocationCount;
    }
}

TEST_CASE("Allocation counter sees a heap allocation (self-test)", "[state][realtime]")
{
#if defined(B33P_ALLOC_COUNTER_DISABLED)
    SKIP("allocation counter is disabled under AddressSanitizer");
#endif
    const long n = allocationsDuring([]
    {
        auto p = std::make_unique<int>(7);
        juce::ignoreUnused(p);
    });
    REQUIRE(n == 1);
}

TEST_CASE("B33pProcessor::processBlock makes no heap allocation while playing, "
          "modulating, applying overrides and taking MIDI",
          "[state][realtime]")
{
#if defined(B33P_ALLOC_COUNTER_DISABLED)
    SKIP("allocation counter is disabled under AddressSanitizer");
#endif
    auto p = std::make_unique<B33pProcessor>();

    // Every lane modulated through the matrix and running a Mod FX, so the
    // whole parameter-read path (raw values, matrix, overrides) is used.
    using FX = B33p::ModulationEffect::Type;
    const FX effects[] = { FX::Chorus, FX::Reverb, FX::Delay, FX::Phaser };
    for (int lane = 0; lane < B33p::Pattern::kNumLanes; ++lane)
    {
        setReal(*p, IDs::modEffectType(lane), static_cast<float>(effects[lane]));
        setReal(*p, IDs::lfoRateHz(lane, 0), 4.0f);
        setReal(*p, IDs::modSlotSource(lane, 0), static_cast<float>(B33p::ModSource::LFO1));
        setReal(*p, IDs::modSlotDest(lane, 0), static_cast<float>(ModDestination::FilterCutoff));
        setReal(*p, IDs::modSlotAmount(lane, 0), 0.5f);
        setReal(*p, IDs::modSlotSource(lane, 1), static_cast<float>(B33p::ModSource::LFO2));
        setReal(*p, IDs::modSlotDest(lane, 1), static_cast<float>(ModDestination::VoiceGain));
        setReal(*p, IDs::modSlotAmount(lane, 1), -0.3f);

        Event e;
        e.startSeconds    = 0.013 * lane;
        e.durationSeconds = 0.05;
        e.overrides[0] = { ModDestination::DistortionDrive, 0.7f };
        e.overrides[1] = { ModDestination::ModEffectMix,    0.4f };
        p->getPattern().addEvent(lane, e);
    }
    p->getPattern().setLengthSeconds(0.25);
    p->setLooping(true);

    constexpr int kBlockSize = 256;
    p->prepareToPlay(48000.0, kBlockSize);
    p->startPlayback();

    juce::AudioBuffer<float> block(2, kBlockSize);
    juce::MidiBuffer midi;
    midi.ensureSize(256);   // the MidiBuffer is host-owned; size it up front

    auto runBlock = [&](int index)
    {
        block.clear();
        midi.clear();
        if (index % 7 == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 60 + index % 12, 0.8f), 37);
        if (index % 7 == 3)
            midi.addEvent(juce::MidiMessage::noteOff(1, 60 + (index - 3) % 12), 101);
        if (index % 31 == 0)
            p->triggerAudition();
        // MidiBuffer::addEvent is the host's side of the contract; only the
        // processBlock call itself is counted.
        return allocationsDuring([&] { p->processBlock(block, midi); });
    };

    // Warm up: lets every voice trigger once so per-voice storage (pitch
    // curve copies) has reached its steady size.
    for (int i = 0; i < 100; ++i)
        runBlock(i);

    long total = 0;
    for (int i = 100; i < 400; ++i)
        total += runBlock(i);

    REQUIRE(total == 0);
}

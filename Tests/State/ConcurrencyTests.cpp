#include <catch2/catch_test_macros.hpp>

#include "Core/ParameterIDs.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <chrono>
#include <random>
#include <thread>
#include <vector>

using B33p::B33pProcessor;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 256;
}

// Smoke test only — not a proof of concurrency-safety. Catches obvious
// breakage of the SpinLock / retire-list patterns: a writer thread
// stomping on the reader's state, a reader dereferencing a destroyed
// shared_ptr, or a crash under sustained contention. The release build
// (no ASan/TSan) just verifies the program doesn't crash; the
// `ubuntu-latest (asan + ubsan)` CI job runs the same suite with
// AddressSanitizer + UndefinedBehaviorSanitizer attached, which catches
// out-of-bounds and use-after-free that this loop would otherwise
// silently survive. TSan is not part of CI today; if it ever is, this
// test's run-count and duration should bump.
TEST_CASE("B33pProcessor: setWavetableSlot concurrent with processBlock survives sustained contention",
          "[state][concurrency]")
{
    B33pProcessor processor;
    processor.prepareToPlay(kSampleRate, kBlockSize);

    std::atomic<bool> stop { false };

    // Writer thread: keeps stuffing the wavetable slots with new vectors.
    // The retire-list pattern guarantees destruction of the previous-
    // previous pointer happens on THIS thread, not the audio thread.
    std::thread writer([&]() {
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> sizeDist(64, 1024);
        std::uniform_real_distribution<float> sampleDist(-1.0f, 1.0f);
        int counter = 0;
        while (! stop.load())
        {
            const int lane = counter % B33p::Pattern::kNumLanes;
            const int slot = counter % B33p::Oscillator::kNumWavetableSlots;
            std::vector<float> samples(static_cast<size_t>(sizeDist(rng)));
            for (auto& s : samples)
                s = sampleDist(rng);
            processor.setWavetableSlot(lane, slot, std::move(samples));
            ++counter;
            // Tight loop with a small yield so the reader can make
            // forward progress under the spin lock.
            std::this_thread::yield();
        }
    });

    // Reader (this thread) drives processBlock in a tight loop. The
    // pushParametersToLane path takes the wavetable spin-try-lock and
    // pulls the latest slots; under contention with the writer it will
    // sometimes fall through to the cached `lastPushedWavetableSlots`.
    juce::AudioBuffer<float> buf(2, kBlockSize);
    juce::MidiBuffer midi;
    constexpr int kBlocksToRun = 2000;   // ~10.7 s of audio at 48 kHz/256
    for (int i = 0; i < kBlocksToRun; ++i)
        processor.processBlock(buf, midi);

    stop.store(true);
    writer.join();

    // If we got here without a crash or sanitizer trip, the test passes.
    SUCCEED("processBlock + setWavetableSlot survived " << kBlocksToRun
            << " blocks of concurrent writes");
}

TEST_CASE("B33pProcessor: refreshPatternSnapshot concurrent with processBlock survives sustained contention",
          "[state][concurrency]")
{
    B33pProcessor processor;
    processor.prepareToPlay(kSampleRate, kBlockSize);

    // Add a couple of events so the snapshot isn't trivially empty.
    auto& pattern = processor.getPattern();
    pattern.addEvent(0, { 0.1, 0.05, 0.0f, 1.0f });
    pattern.addEvent(1, { 0.3, 0.10, 0.0f, 1.0f });

    // Start playback so the audio thread's snapshot-read path is hot.
    processor.startPlayback();

    std::atomic<bool> stop { false };

    std::thread writer([&]() {
        while (! stop.load())
        {
            processor.refreshPatternSnapshot();
            std::this_thread::yield();
        }
    });

    juce::AudioBuffer<float> buf(2, kBlockSize);
    juce::MidiBuffer midi;
    constexpr int kBlocksToRun = 2000;
    for (int i = 0; i < kBlocksToRun; ++i)
        processor.processBlock(buf, midi);

    stop.store(true);
    writer.join();
    processor.stopPlayback();

    SUCCEED("processBlock + refreshPatternSnapshot survived " << kBlocksToRun
            << " blocks of concurrent writes");
}

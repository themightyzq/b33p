#pragma once

#include "B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace B33p
{
    // Offline pattern render for the export dialog. Drives a private
    // B33pProcessor (the same engine as live playback: voices, filter
    // modes, Mod FX, LFO matrix, per-event overrides, output limiter)
    // through processBlock, so the exported audio is what playback
    // produces. The live processor is only read, once, on the message
    // thread (captureState); every render and every batch dice roll
    // happens on the private instance.
    //
    // Differences from live playback, all deliberate:
    //   * Looping, host-transport follow and host bypass are forced off:
    //     an export always plays the pattern once from 0.
    //   * Random sources (probability, humanize, Noise) are seeded, so
    //     one project always renders the same file. Live playback is
    //     not seeded and varies run to run; an export is one
    //     realisation of it.
    //   * Parameters and LFOs update once per processBlock call, so the
    //     result depends on block size; exports always use kBlockSize.
    //
    // Threading: construct and destroy on the message thread (the
    // processor's APVTS owns a Timer); render() / renderVariation() may
    // run on a background thread. The instance must not be used from
    // two threads at once.
    class OfflineExporter
    {
    public:
        static constexpr int         kBlockSize          = 512;
        static constexpr juce::int64 kDefaultSeed        = 0x62333370;   // "b33p"

        // Hard cap on the rendered tail after the pattern ends. Matches
        // B33pProcessor::getTailLengthSeconds().
        static constexpr double      kMaxTailSeconds     = 10.0;

        // The tail ends once the output has stayed below the threshold
        // for this long. Longer than the 2 s maximum Delay time, so the
        // gap between two echoes is not mistaken for the end.
        static constexpr double      kSilenceHoldSeconds = 2.5;
        static constexpr float       kSilenceThreshold   = 1.0e-5f;  // -100 dBFS

        struct Settings
        {
            double      sampleRate     { 48000.0 };
            int         blockSize      { kBlockSize };
            juce::int64 seed           { kDefaultSeed };
            double      maxTailSeconds { kMaxTailSeconds };
        };

        // Serialises the live processor's full state (parameters,
        // pattern, pitch curve, wavetables, locks) through the same
        // path a DAW session save uses. Message thread.
        static juce::MemoryBlock captureState(B33pProcessor& live);

        // Builds the private processor and loads `state` into it.
        explicit OfflineExporter(const juce::MemoryBlock& state);

        // Plays the pattern once and returns the mono mix. The output
        // starts at pattern time 0 (the oversampling latency is
        // trimmed), is at least the pattern length, and runs on
        // through the release / effect tail until it falls silent,
        // capped at maxTailSeconds past the pattern end.
        juce::AudioBuffer<float> render(const Settings& settings);

        // Batch export step. Variation 0 renders the captured patch
        // unchanged; every later variation first rolls the private
        // instance's unlocked parameters with rollRng. Each variation
        // uses seed settings.seed + variationIndex.
        //
        // Pass the calling thread when calling from a background thread:
        // the roll then runs under a MessageManagerLock, because it opens
        // an undo transaction on the private APVTS's UndoManager, which
        // the APVTS flush timer also writes to on the message thread.
        // Returns an empty buffer if the lock could not be gained (the
        // thread was asked to exit).
        juce::AudioBuffer<float> renderVariation(int variationIndex,
                                                 const Settings& settings,
                                                 juce::Random& rollRng,
                                                 juce::Thread* callingThread = nullptr);

        B33pProcessor&       getProcessor()       noexcept { return *processor; }
        const B33pProcessor& getProcessor() const noexcept { return *processor; }

    private:
        std::unique_ptr<B33pProcessor> processor;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OfflineExporter)
    };
}

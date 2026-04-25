#pragma once

#include "DSP/Voice.h"
#include "Pattern/Pattern.h"
#include "Pattern/PatternSnapshot.h"
#include "ParameterRandomizer.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <memory>
#include <vector>

namespace B33p
{
    // First-class juce::AudioProcessor for the standalone B33p app.
    // Owns the APVTS (built from createParameterLayout()), an
    // UndoManager that the APVTS routes parameter edits through, a
    // Voice that processBlock drives in real time, the user's
    // pitch envelope curve, the pattern data model, and the runtime
    // playback state for the pattern sequencer.
    //
    // Concurrency:
    //   * APVTS values are atomic floats; safe for both threads.
    //   * pitchCurve is protected by a CriticalSection; the audio
    //     thread reads via ScopedTryLock and falls back to whatever
    //     curve the Voice already has when the lock is contended.
    //   * Pattern playback uses a snapshot pattern: when the user
    //     starts playback, the message thread builds a flat sorted
    //     PatternSnapshot, atomically swaps the shared_ptr slot,
    //     then sets isPlaying. The audio thread atomic_loads the
    //     pointer and holds the ref for the whole block, so
    //     concurrent re-snapshots cannot corrupt mid-block reads.
    //
    // Save / load (getStateInformation / setStateInformation) is
    // stubbed out — that lands in Phase 6 with the .beep file format.
    class B33pProcessor : public juce::AudioProcessor
    {
    public:
        B33pProcessor();

        juce::AudioProcessorValueTreeState&       getApvts()       { return apvts; }
        const juce::AudioProcessorValueTreeState& getApvts() const { return apvts; }

        juce::UndoManager& getUndoManager() { return undoManager; }

        ParameterRandomizer&       getRandomizer()       { return randomizer; }
        const ParameterRandomizer& getRandomizer() const { return randomizer; }

        // Message-thread entry point. Sets a flag that the next
        // processBlock picks up to call voice.trigger() + schedule
        // an auto-noteOff after the audition duration elapses.
        void triggerAudition();

        // The drawn pitch envelope curve. Unlike the other parameters,
        // the curve is not an APVTS value — it is a variable-length
        // breakpoint list and will move into the project ValueTree in
        // Phase 6 when save/load arrives.
        const std::vector<PitchEnvelopePoint>& getPitchCurve() const { return pitchCurve; }
        void setPitchCurve(std::vector<PitchEnvelopePoint> newCurve);

        // The user-authored sequencer pattern. UI edits go through
        // this reference directly. Playback uses an immutable
        // snapshot built when startPlayback() runs, so live UI edits
        // do not affect playback in progress.
        Pattern&       getPattern()       { return pattern; }
        const Pattern& getPattern() const { return pattern; }

        // Pattern-playback control (message-thread). startPlayback
        // builds a snapshot of the current pattern and arms the
        // audio thread; stopPlayback releases the voice and clears
        // the playing flag.
        void   startPlayback();
        void   stopPlayback();
        bool   isPlaying() const     { return playing.load(std::memory_order_acquire); }

        void   setLooping(bool shouldLoop) { looping.store(shouldLoop); }
        bool   getLooping() const          { return looping.load(); }

        // Audio thread writes once per block, UI reads from a Timer
        // callback. atomic<double> is lock-free on every 64-bit
        // platform we support.
        double getPlayheadSeconds() const  { return playheadSeconds.load(); }

        // juce::AudioProcessor interface ----------------------------
        const juce::String getName() const override                              { return "B33p"; }
        void  prepareToPlay(double sampleRate, int blockSize) override;
        void  releaseResources() override;
        void  processBlock(juce::AudioBuffer<float>& buffer,
                           juce::MidiBuffer& midi) override;

        bool  hasEditor() const override                                         { return false; }
        juce::AudioProcessorEditor* createEditor() override                      { return nullptr; }

        bool  acceptsMidi() const override                                       { return false; }
        bool  producesMidi() const override                                      { return false; }
        double getTailLengthSeconds() const override                             { return 0.0; }

        int   getNumPrograms() override                                          { return 1; }
        int   getCurrentProgram() override                                       { return 0; }
        void  setCurrentProgram(int) override                                    {}
        const juce::String getProgramName(int) override                          { return {}; }
        void  changeProgramName(int, const juce::String&) override               {}

        // Phase 6 will fill these out via ValueTree round-trip.
        void  getStateInformation(juce::MemoryBlock&) override                   {}
        void  setStateInformation(const void*, int) override                     {}

    private:
        void pushParametersToVoice();
        void triggerVoiceFromEvent(const Event& event);

        juce::UndoManager                  undoManager;
        juce::AudioProcessorValueTreeState apvts;
        ParameterRandomizer                randomizer;

        Voice voice;

        // Curve writes lock on the message thread; audio-thread reads
        // use a ScopedTryLock and fall back to whatever curve the Voice
        // already has. The race window is tiny but real once live audio
        // is wired.
        juce::CriticalSection pitchCurveLock;
        std::vector<PitchEnvelopePoint> pitchCurve { { 0.0f, 0.0f }, { 1.0f, 0.0f } };

        Pattern pattern;

        // ---- Playback / audition state shared across threads ------
        std::atomic<bool>   pendingAudition  { false };
        std::atomic<bool>   playing          { false };
        std::atomic<bool>   looping          { true  };
        std::atomic<double> playheadSeconds  { 0.0   };

        // Pattern snapshot lives behind atomic_load/store — see
        // class-level comment for the threading model.
        std::shared_ptr<const PatternSnapshot> snapshotSlot;

        // Audio-thread mutable state (touched only from processBlock).
        bool                                   audioThreadPlaying { false };
        std::shared_ptr<const PatternSnapshot> activeSnapshot;
        int                                    nextEventIndex     { 0 };
        int                                    samplesUntilNoteOff { 0 };
        double                                 currentSampleRate   { 44100.0 };
    };
}

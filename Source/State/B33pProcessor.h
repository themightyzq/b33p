#pragma once

#include "DSP/Voice.h"
#include "Pattern/Pattern.h"
#include "Pattern/PatternSnapshot.h"
#include "ParameterRandomizer.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <functional>
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
                        , private juce::AudioProcessorValueTreeState::Listener
    {
    public:
        B33pProcessor();
        ~B33pProcessor() override;

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

        // Thread-safe snapshot for the offline export thread — takes
        // the same CriticalSection the audio-thread try-lock uses, so
        // a copy here cannot race with a concurrent UI write.
        std::vector<PitchEnvelopePoint> getPitchCurveCopy() const;

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

        // Rebuild the pattern snapshot from the live Pattern and
        // atomic-store it into the slot the audio thread reads.
        // Cheap (a copy + sort of a small vector) and safe to call
        // any time. Used by the message-thread timer to push live
        // pattern edits into a running playback session.
        void   refreshPatternSnapshot();

        void   setLooping(bool shouldLoop);
        bool   getLooping() const          { return looping.load(); }

        // Unsaved-changes tracking. Anything that mutates persisted
        // project state (APVTS values, pitch curve, pattern, locks)
        // marks the processor dirty; saving / loading clears it.
        // The callback fires on dirty<->clean transitions only,
        // marshalled to the message thread via callAsync so the UI
        // can update safely regardless of which thread mutated.
        bool isDirty() const noexcept { return dirty.load(); }
        void markDirty();
        void markClean();

        using OnDirtyChanged = std::function<void()>;
        void setOnDirtyChanged(OnDirtyChanged callback);

        // Fires after a bulk state replacement (Open or New) so the
        // UI can resync widgets that don't auto-track APVTS — pitch
        // editor, pattern grid, pattern length / grid combos, etc.
        // Marshalled to the message thread so it's safe to call from
        // setStateInformation regardless of the calling thread.
        using OnFullStateLoaded = std::function<void()>;
        void setOnFullStateLoaded(OnFullStateLoaded callback);
        void notifyFullStateLoaded();

        // Resets every persisted piece of state (APVTS, pitch curve,
        // pattern, locks, undo history) back to the same values a
        // freshly-launched app would show, then markClean()s and
        // fires OnFullStateLoaded for the UI to resync.
        void resetToDefaults();

        // Which lane the editor sections currently target. The audio
        // thread reads this for audition routing; the UI reads it
        // when rebuilding section attachments. Setter clamps to
        // [0, kNumLanes) and fires OnSelectedLaneChanged on transition.
        int  getSelectedLane() const { return selectedLane.load(); }
        void setSelectedLane(int lane);

        using OnSelectedLaneChanged = std::function<void(int newLane)>;
        void setOnSelectedLaneChanged(OnSelectedLaneChanged callback);

        // " (Lane N)" or " (Lane N: Body)" depending on whether the
        // pattern lane has a custom name. Used by the editor sections
        // to telegraph which lane they're currently editing.
        juce::String laneTitleSuffix(int lane) const;

        // Copies every voice parameter from sourceLane onto all the
        // other lanes (or just one destLane if specified). One
        // undoable transaction per call.
        void copyLaneSettingsToAll(int sourceLane);

        // Resets one lane's voice parameters to their defaults.
        // One undoable transaction.
        void resetLaneVoice(int lane);

        // Audio thread writes once per block, UI reads from a Timer
        // callback. atomic<double> is lock-free on every 64-bit
        // platform we support.
        double getPlayheadSeconds() const  { return playheadSeconds.load(); }

        // Lets the UI park the playhead at a specific time (e.g. by
        // clicking on the pattern grid ruler). Clamped to
        // [0, pattern length). Used as the paste-target time for
        // Cmd+V even when playback is stopped.
        void   setPlayheadSeconds(double seconds);

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

        // ValueTree-backed save/load via ProjectState. Hosts and
        // the .beep file format share the same XML serialization.
        void  getStateInformation(juce::MemoryBlock& destData) override;
        void  setStateInformation(const void* data, int sizeInBytes) override;

    private:
        void pushParametersToVoices();
        void pushParametersToLane(int lane);
        void triggerVoiceFromEvent(int lane, const Event& event);

        // juce::AudioProcessorValueTreeState::Listener
        void parameterChanged(const juce::String& parameterID, float newValue) override;

        void registerAsApvtsListener();
        void unregisterAsApvtsListener();
        void notifyDirtyChanged();

        juce::UndoManager                  undoManager;
        juce::AudioProcessorValueTreeState apvts;
        ParameterRandomizer                randomizer;

        // One Voice per pattern lane. Each lane's events trigger
        // its own voice; the pattern engine sums all four voices
        // each sample.
        std::array<Voice, Pattern::kNumLanes> voices;

        // Curve writes lock on the message thread; audio-thread reads
        // use a ScopedTryLock and fall back to whatever curve each
        // voice already has. One curve is shared across all four
        // voices for now — per-lane pitch curves are post-MVP.
        mutable juce::CriticalSection pitchCurveLock;
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
        // Per-lane sample-accurate noteOff countdowns. 0 = no
        // pending noteOff; > 0 counts down each sample.
        std::array<int, Pattern::kNumLanes>    samplesUntilNoteOff { { 0, 0, 0, 0 } };
        double                                 currentSampleRate   { 44100.0 };

        // Unsaved-changes flag + listener.
        std::atomic<bool>     dirty                       { false };
        OnDirtyChanged        onDirtyChangedCallback;
        OnFullStateLoaded     onFullStateLoadedCallback;

        // Lane the editor UI currently targets. Atomic because the
        // audio thread reads it when routing the audition trigger.
        std::atomic<int>      selectedLane                { 0 };
        OnSelectedLaneChanged onSelectedLaneChangedCallback;
    };
}

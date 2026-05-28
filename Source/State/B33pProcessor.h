#pragma once

#include "DSP/LFO.h"
#include "DSP/ModulationMatrix.h"
#include "DSP/OutputLimiter.h"
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

        // Audible-randomize rule: the randomizer rolls each param
        // independently with no whole-patch check, so silence can
        // emerge from combinations (filter closed, amp sustain ~0,
        // Mod FX wet kills the dry). Call this from every UI
        // randomize entry point AFTER the roll, once per touched
        // lane: it offline-renders the audition, and if the patch is
        // near-silent it re-rolls the level-critical parameters up
        // to a few times and falls back to known-audible safe values
        // as a last resort. See AudibilityCheck + project memory
        // [[rule-randomize-must-be-audible]] for the rule's intent.
        void ensureLaneAudibleAfterRandomize(int lane, juce::Random& rng);

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

        // Per-lane wavetable storage. Each lane owns
        // Oscillator::kNumWavetableSlots single-cycle tables. Slot 0
        // doubles as the Custom-mode table — Custom and Wavetable
        // modes share the same storage to keep the editor and the
        // saved-file schema simple.
        //
        // Set is non-blocking via atomic shared_ptr swap; get returns
        // a value copy of the currently-published slot (or an empty
        // vector if none has ever been set).
        void                setCustomWaveform(int lane,
                                              std::vector<float> samples);
        std::vector<float>  getCustomWaveformCopy(int lane) const;

        void                setWavetableSlot(int lane, int slot,
                                             std::vector<float> samples);
        std::vector<float>  getWavetableSlotCopy(int lane, int slot) const;

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

        // When running as a VST3 / AU plugin, the host provides a
        // playhead. Setting follow=true makes b33p's pattern
        // playback mirror host transport: host play / stop drives
        // our `playing` flag and the playhead position becomes
        // (host_time % patternLength). Pattern BPM is intentionally
        // NOT synced — sound-design users routinely want a beep
        // pattern that runs at a different tempo from the host
        // session it's layered into.
        //
        // In standalone mode (or plugin contexts that don't expose
        // a playhead), this setting is inert and pattern playback
        // uses the existing internal transport.
        void   setFollowHostTransport(bool shouldFollow);
        bool   getFollowHostTransport() const { return followHostTransport.load(); }

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

        // Last editor size (points), persisted in the plugin's DAW state so
        // a resized window survives close / reopen (REVIEW.md P11). Stored on
        // the processor because the editor is transient; 0×0 means "never
        // set, use the default". Not part of ProjectState / .beep — window
        // size is session state, not portable patch data. Resizing the editor
        // does NOT mark the project dirty.
        int  getEditorWidth()  const { return editorWidth.load(); }
        int  getEditorHeight() const { return editorHeight.load(); }
        void setEditorSize(int width, int height)
        {
            editorWidth.store(width);
            editorHeight.store(height);
        }

        // " (Lane N)" or " (Lane N: Body)" depending on whether the
        // pattern lane has a custom name. Used by the editor sections
        // to telegraph which lane they're currently editing.
        juce::String laneTitleSuffix(int lane) const;

        // Per-lane base accent colour. Used by the voice editor
        // sections (title accent bar) and the pattern grid (lane
        // row tint) so a fresh user can see which lane is which
        // without reading the suffix every time.
        juce::Colour laneAccentColour(int lane) const;

        // Copies every voice parameter from sourceLane onto all the
        // other lanes (or just one destLane if specified). One
        // undoable transaction per call.
        void copyLaneSettingsToAll(int sourceLane);

        // Copy / paste a single lane's whole voice (every APVTS param for the
        // lane + its wavetable slots) as portable text, for moving a dialed-in
        // voice between lanes or between plugin instances via the system
        // clipboard (REVIEW.md P24). copyLaneVoiceToString serialises;
        // applyLaneVoiceFromString applies to the target lane in one undo
        // transaction and returns false if the text isn't a valid b33p voice
        // (e.g. an unrelated clipboard, or a future-version shape).
        juce::String copyLaneVoiceToString(int lane) const;
        bool         applyLaneVoiceFromString(int lane, const juce::String& data);

        // Resets one lane's voice parameters to their defaults.
        // One undoable transaction.
        void resetLaneVoice(int lane);

        // ---- A/B compare -----------------------------------------
        // Two-slot snapshot/compare of APVTS state. Snapshots store
        // parameter values only (not Pattern, pitch curve, or
        // wavetables); A/B is for comparing patches, not songs.
        //
        // First switch to B copies A's current state into B so the
        // user has something to start tweaking from instead of an
        // empty B-side. Subsequent switches save the leaving side
        // and load the arriving side. Both snapshots + active side
        // are persisted to the .beep / DAW project state (v13).
        char getActiveAbSlot() const noexcept { return activeAbSlot; }
        void switchAbSlot(char target);
        void copyActiveAbSlotToOther();

        // ProjectState (v13+) serialises snapshots via these. Caller
        // owns the returned pointer's data (deep-copy as needed).
        const juce::XmlElement* getAbSnapshot(char slot) const noexcept;
        void restoreAbState(char activeSlot,
                            std::unique_ptr<juce::XmlElement> snapshotA,
                            std::unique_ptr<juce::XmlElement> snapshotB);

        // Audio thread writes once per block, UI reads from a Timer
        // callback. atomic<double> is lock-free on every 64-bit
        // platform we support.
        double getPlayheadSeconds() const  { return playheadSeconds.load(); }

        // Most recent output-buffer peak (0..1). Audio thread writes
        // once per block with a small per-block decay so the meter
        // falls when the signal stops; UI reads via Timer.
        float  getOutputPeak() const  { return outputPeak.load(); }

        // Current output (-1..1) of the selected lane's two free-running
        // LFOs. Audio thread writes once per block; the modulation editor
        // reads via Timer to show a live "this routing is active" cue
        // (REVIEW.md P14). Index out of range returns 0.
        float  getSelectedLaneLfoValue(int lfoIndex) const
        {
            return (lfoIndex >= 0 && lfoIndex < kNumLfosPerLane)
                       ? selectedLaneLfoValues[static_cast<size_t>(lfoIndex)].load()
                       : 0.0f;
        }

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

        // hasEditor / createEditor are gated on the B33P_HAS_EDITOR
        // compile flag set by the plugin / standalone target. The
        // tests target builds the same B33pProcessor.cpp without
        // the editor sources (which would drag the entire UI layer
        // into a non-UI test binary), so it gets the no-editor
        // branch automatically.
        bool                        hasEditor()    const override;
        juce::AudioProcessorEditor* createEditor()       override;

        bool  acceptsMidi() const override                                       { return true;  }
        bool  producesMidi() const override                                      { return false; }
        // Covers worst-case audible tail across the per-voice signal
        // path: amp-envelope release (~1 s max), reverb decay (~3 s
        // at large-room settings), and delay feedback echoes (~2-3 s
        // at moderate feedback). Conservative single value rather
        // than per-lane dynamic computation — the host uses this to
        // decide how many extra samples to record after transport
        // stop so a reverb / delay tail isn't cut off mid-decay.
        double getTailLengthSeconds() const override                             { return 4.0; }

        // Host bypass — DAWs use this to route their bypass automation
        // to a parameter we own. processBlock checks bypassParam and
        // clears the buffer when set, so bypass actually does what the
        // user expects (silence) instead of letting voices play
        // through.
        juce::AudioProcessorParameter* getBypassParameter() const override;

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
        // Lower-level worker: push the lane's APVTS values + matrix
        // modulation into the supplied Voice. applyOverrides toggles
        // whether per-event override slots win; MIDI keyboard voices
        // pass false because they aren't event-driven.
        void pushParametersToVoiceImpl(int lane, Voice& v, bool applyOverrides);
        void triggerVoiceFromEvent(int lane, const Event& event);

        // Reads APVTS LFO params + modulation-matrix slots for the
        // lane and updates each LFO's runtime config; computes the
        // total modulation contribution per destination so
        // pushParametersToLane can apply it on top of the base
        // APVTS values before pushing to the Voice.
        // Returns an array indexed by ModDestination (cast to int).
        std::array<float, kNumModDestinations>
            evaluateModulationContributions(int lane);

        // Returns the APVTS-base value for a destination param,
        // shifted by the given modulation amount in the parameter's
        // normalised [0..1] space (or by +/- 12 semitones for the
        // pitch destination). Used by pushParametersToLane.
        float effectiveParamValue(int lane,
                                  ModDestination destination,
                                  float modAmount) const;

        // If any active override slot for `lane` targets the given
        // destination, returns the converted (natural-range) value;
        // otherwise returns false. The first matching slot wins —
        // duplicate-destination slots are a UI bug, not handled
        // specially here.
        bool tryGetActiveOverride(int lane,
                                  ModDestination destination,
                                  float& outValue) const;

        // juce::AudioProcessorValueTreeState::Listener
        void parameterChanged(const juce::String& parameterID, float newValue) override;

        void registerAsApvtsListener();
        void unregisterAsApvtsListener();
        void notifyDirtyChanged();

        juce::UndoManager                  undoManager;
        juce::AudioProcessorValueTreeState apvts;
        ParameterRandomizer                randomizer;

        // Number of polyphonic voices reserved for MIDI keyboard
        // play. Pattern playback stays monophonic per lane (events
        // re-trigger the same voice with the amp-env-retrigger
        // policy from CLAUDE.md); MIDI voices live in their own
        // pool so a user playing a chord doesn't fight the pattern
        // for voice slots.
        static constexpr int kMidiPolyphony = 8;

        // One Voice per pattern lane. Each lane's events trigger
        // its own voice; the pattern engine sums all four voices
        // each sample.
        std::array<Voice, Pattern::kNumLanes> voices;

        // MIDI keyboard voice pool. Allocated round-robin on
        // note-on, released on note-off via the midiNoteToVoice
        // lookup. Always inherits the currently-selected lane's
        // parameters so the MIDI player is sound-designing the
        // selected lane in real time.
        std::array<Voice, kMidiPolyphony>     midiVoices;
        std::array<int,   128>                midiNoteToVoice;     // -1 = note not held
        int                                   nextMidiVoiceIndex { 0 };

        // Two free-running LFOs per lane. Phase advances once per
        // audio block (block-rate modulation) — pushParametersToLane
        // reads currentValue() before deciding which Voice setters
        // to call with modulated values.
        std::array<std::array<LFO, kNumLfosPerLane>, Pattern::kNumLanes> lfos;

        // Soft-clip safety on the summed output (voices sum with no
        // headroom, so a dense / randomized pattern could hard-clip).
        OutputLimiter outputLimiter;

        // Per-lane "currently active per-event overrides" mirror.
        // Loaded from Event::overrides in triggerVoiceFromEvent and
        // consulted in pushParametersToLane. Override slots with
        // destination == None are inert. Cleared per trigger so a
        // new event's overrides fully replace the previous event's.
        std::array<std::array<EventOverride, kNumEventOverrides>,
                   Pattern::kNumLanes>             activeOverrides {};

        // Source of randomness for snapshot-time probability rolls,
        // ratchet expansion humanize jitter, and any other future
        // randomised pattern features. Re-seeded by the system on
        // construction; unseeded re-snapshots will pick up wherever
        // the previous call left off.
        juce::Random snapshotRng;

        // Curve writes lock on the message thread; audio-thread reads
        // use a ScopedTryLock and fall back to whatever curve each
        // voice already has. One curve is shared across all four
        // voices — per-lane pitch curves are a deferred item.
        mutable juce::CriticalSection pitchCurveLock;
        std::vector<PitchEnvelopePoint> pitchCurve { { 0.0f, 0.0f }, { 1.0f, 0.0f } };

        Pattern pattern;

        // ---- Playback / audition state shared across threads ------
        std::atomic<bool>   pendingAudition       { false };
        std::atomic<bool>   playing               { false };
        std::atomic<bool>   looping               { true  };
        std::atomic<bool>   followHostTransport   { false };
        std::atomic<double> playheadSeconds       { 0.0   };
        std::atomic<float>  outputPeak            { 0.0f  };
        // Live LFO outputs for the selected lane (P14 mod-activity cue).
        std::array<std::atomic<float>, kNumLfosPerLane> selectedLaneLfoValues {};

        // Pattern snapshot lives behind atomic_load/store — see
        // class-level comment for the threading model.
        std::shared_ptr<const PatternSnapshot> snapshotSlot;

        // Cached raw-parameter pointer for host bypass. Set once in
        // the constructor from apvts.getRawParameterValue(); checked
        // every processBlock to decide whether to clear and bail.
        std::atomic<float>*                    bypassParam { nullptr };

        // A/B slot snapshots — APVTS state XML for each side. Both
        // can be nullptr (then a switch from A → B captures the
        // current state into A and copies it into B). activeAbSlot
        // is the side whose values currently drive the APVTS.
        std::unique_ptr<juce::XmlElement>      abSnapshotA;
        std::unique_ptr<juce::XmlElement>      abSnapshotB;
        char                                   activeAbSlot { 'A' };

        // Audio-thread mutable state (touched only from processBlock).
        bool                                   audioThreadPlaying { false };
        std::shared_ptr<const PatternSnapshot> activeSnapshot;
        int                                    nextEventIndex     { 0 };
        // Per-lane sample-accurate noteOff countdowns. 0 = no
        // pending noteOff; > 0 counts down each sample.
        std::array<int, Pattern::kNumLanes>    samplesUntilNoteOff { { 0, 0, 0, 0 } };
        double                                 currentSampleRate   { 44100.0 };

        // Per-lane wavetable slot storage. Message thread writes via
        // atomic shared_ptr store; audio thread loads each block and
        // pushes the corresponding slot to the voice only if the
        // pointer changed since last push (avoids per-block vector
        // copies when the user isn't editing). One inner array per
        // lane, one shared_ptr per slot. Custom mode reads slot 0;
        // Wavetable mode reads all four blended by the morph param.
        using WavetableSlotPtr = std::shared_ptr<const std::vector<float>>;
        using WavetableSlotArray = std::array<WavetableSlotPtr,
                                              Oscillator::kNumWavetableSlots>;

        std::array<WavetableSlotArray, Pattern::kNumLanes> wavetableSlots;
        std::array<WavetableSlotArray, Pattern::kNumLanes> lastPushedWavetableSlots;

        // Unsaved-changes flag + listener.
        std::atomic<bool>     dirty                       { false };
        OnDirtyChanged        onDirtyChangedCallback;
        OnFullStateLoaded     onFullStateLoadedCallback;

        // Lane the editor UI currently targets. Atomic because the
        // audio thread reads it when routing the audition trigger.
        std::atomic<int>      selectedLane                { 0 };
        OnSelectedLaneChanged onSelectedLaneChangedCallback;

        // Last editor size in points (0 = unset → editor uses its default).
        // Atomic since a host may call getStateInformation off the message
        // thread while the editor writes from the message thread.
        std::atomic<int>      editorWidth                 { 0 };
        std::atomic<int>      editorHeight                { 0 };
    };
}

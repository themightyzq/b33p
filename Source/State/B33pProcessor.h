#pragma once

#include "DSP/Voice.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace B33p
{
    // First-class juce::AudioProcessor for the standalone B33p app.
    // Owns the APVTS (built from createParameterLayout()), an
    // UndoManager that the APVTS routes parameter edits through, and
    // a Voice that later phases will drive from processBlock when an
    // audio device is wired up.
    //
    // For this MVP step processBlock is intentionally silent: there
    // is no audio device callback yet, no audition path, no pattern
    // sequencer. The processor exists so the editor can attach
    // sliders / dropdowns to APVTS parameters via the standard
    // SliderAttachment / ComboBoxAttachment APIs.
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

        // Message-thread entry point. Sets a flag that the next
        // processBlock picks up to call voice.trigger() + schedule
        // an auto-noteOff after the audition duration elapses.
        void triggerAudition();

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

        // The drawn pitch envelope curve. Unlike the other parameters,
        // the curve is not an APVTS value — it is a variable-length
        // breakpoint list and will move into the project ValueTree in
        // Phase 6 when save/load arrives.
        const std::vector<PitchEnvelopePoint>& getPitchCurve() const { return pitchCurve; }
        void setPitchCurve(std::vector<PitchEnvelopePoint> newCurve);

    private:
        void pushParametersToVoice();

        juce::UndoManager                  undoManager;
        juce::AudioProcessorValueTreeState apvts;

        Voice voice;

        // Curve writes lock on the message thread; audio-thread reads
        // use a ScopedTryLock and fall back to whatever curve the Voice
        // already has. The race window is tiny but real once live audio
        // is wired.
        juce::CriticalSection pitchCurveLock;
        std::vector<PitchEnvelopePoint> pitchCurve { { 0.0f, 0.0f }, { 1.0f, 0.0f } };

        std::atomic<bool> pendingAudition { false };
        int samplesUntilAuditionRelease { 0 };
        double currentSampleRate { 44100.0 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(B33pProcessor)
    };
}

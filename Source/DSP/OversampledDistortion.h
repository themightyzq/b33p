#pragma once

#include "Distortion.h"
#include "OversamplingConfig.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace B33p
{
    // Distortion (the tanh waveshaper), wrapped with kOversamplingFactor
    // (4x) oversampling so the harmonics tanh() generates above the
    // input's own bandwidth are pushed above the audible band and
    // filtered off during decimation, instead of folding straight back
    // down into it -- the classic waveshaper-aliasing problem.
    //
    // Distortion itself is left exactly as it was: a plain, latency-free,
    // memoryless class whose own unit tests assert immediate transfer-
    // function values. This class is the only thing that adds the
    // juce::dsp::Oversampling (half-band polyphase IIR) up/down pair and
    // the resulting few samples of latency (getLatencySamples()). Voice
    // owns one of these per lane/voice, matching how Distortion used to
    // be owned directly.
    //
    // Real-time safety: the only allocation is inside
    // juce::dsp::Oversampling::initProcessing(), called from prepare()
    // (i.e. from prepareToPlay), never from processSample().
    //
    // Lifecycle mirrors Distortion: construct -> prepare(sampleRate) ->
    // setDrive -> processSample ... . Before prepare(), processSample()
    // returns silence (0.0f), matching Distortion's own contract.
    class OversampledDistortion
    {
    public:
        OversampledDistortion();

        void prepare(double sampleRate);
        void reset();

        void setDrive(float drive);

        float processSample(float input);

        // Rounded latency (samples) added by the oversampling filters.
        // Deterministic given kOversamplingFactor -- does not depend on
        // drive or sample rate (juce::dsp::Oversampling's filter design
        // takes no sample-rate parameter). 0 before prepare() or with
        // oversampling disabled for testing.
        int getLatencySamples() const;

        // Test-only A/B hook (Tests/DSP/OversamplingAliasingTests.cpp):
        // false collapses this wrapper to a straight passthrough to
        // Distortion::processSample(), i.e. the pre-oversampling (1x)
        // behaviour, so the alias-reduction measurement has a same-
        // process baseline to compare against. Defaults to true
        // (kOversamplingFactor's 4x). Production code (Voice) never
        // calls this -- there is no user-facing toggle.
        void setOversamplingEnabledForTests(bool enabled);

    private:
        Distortion                     distortion;
        juce::dsp::Oversampling<float> oversampler;
        // Fixed 1-sample-per-channel scratch buffers for the up/down
        // AudioBlocks -- Voice submits one sample per processSample()
        // call, so processSamplesUp/Down are always called with a
        // 1-sample block. Sized once here (construction / prepare never
        // resizes in the audio path), never in processSample().
        juce::AudioBuffer<float>       osIn  { 1, 1 };
        juce::AudioBuffer<float>       osOut { 1, 1 };
        bool                           prepared            { false };
        bool                           oversamplingEnabled { true };
    };
}

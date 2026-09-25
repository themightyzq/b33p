#pragma once

#include "Bitcrush.h"
#include "OversamplingConfig.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace B33p
{
    // Bitcrush (sample-and-hold rate reduction + bit-depth quantizer),
    // wrapped with kOversamplingFactor (4x) oversampling. Both the
    // zero-order hold and the quantizer generate energy above the
    // signal's own bandwidth; running them at 4x the sample rate and
    // decimating back down with a proper anti-alias filter pushes that
    // energy above the audible band instead of letting it fold straight
    // back into it.
    //
    // Bitcrush itself is left exactly as it was: a plain, latency-free
    // class whose own unit tests assert immediate sample-and-hold
    // values. This class is the only thing that adds the
    // juce::dsp::Oversampling (half-band polyphase IIR) up/down pair,
    // the resulting few samples of latency (getLatencySamples()), and
    // the factor-scaled hold counter (see processSample()'s comment).
    // Voice owns one of these per lane/voice, matching how Bitcrush used
    // to be owned directly.
    //
    // Real-time safety: the only allocation is inside
    // juce::dsp::Oversampling::initProcessing(), called from prepare()
    // (i.e. from prepareToPlay), never from processSample().
    //
    // Lifecycle mirrors Bitcrush: construct -> prepare(sampleRate) ->
    // setBitDepth / setTargetSampleRate -> processSample ... . Before
    // prepare(), processSample() returns silence (0.0f), matching
    // Bitcrush's own contract.
    class OversampledBitcrush
    {
    public:
        OversampledBitcrush();

        void prepare(double sampleRate);
        void reset();

        void setBitDepth(float bits);
        void setTargetSampleRate(float targetHz);

        // Processes numSamples samples in place (data[0..numSamples) is
        // both input and output). Equivalent sample-for-sample to calling
        // processSample() numSamples times, but runs the oversampler's
        // up/down filter pair once per call instead of once per sample --
        // amortizing juce::dsp::Oversampling's per-call overhead across
        // the whole span. Internally chunks at kMaxOversampledBlockSize
        // (OversamplingConfig.h), so numSamples beyond that ceiling still
        // processes correctly with no extra allocation. Preallocated
        // scratch buffers only -- safe to call from the audio thread.
        void processBlock(float* data, int numSamples);

        // Convenience single-sample form, implemented as
        // processBlock(&input, 1) so it is guaranteed identical to the
        // block path rather than a second, potentially-diverging
        // implementation. Kept for callers that don't batch (unit tests,
        // the B33pRenderVoice CLI).
        float processSample(float input);

        // Rounded latency (samples) added by the oversampling filters.
        // Deterministic given kOversamplingFactor -- does not depend on
        // bit depth, target rate, or sample rate (juce::dsp::
        // Oversampling's filter design takes no sample-rate parameter).
        // 0 before prepare() or with oversampling disabled for testing.
        int getLatencySamples() const;

        // Test-only A/B hook (Tests/DSP/OversamplingAliasingTests.cpp):
        // false collapses this wrapper to a straight passthrough to
        // Bitcrush::processSample() at the host rate, i.e. the pre-
        // oversampling (1x) behaviour, so the alias-reduction
        // measurement has a same-process baseline to compare against.
        // Defaults to true (kOversamplingFactor's 4x). Production code
        // (Voice) never calls this -- there is no user-facing toggle.
        void setOversamplingEnabledForTests(bool enabled);

    private:
        Bitcrush                       bitcrush;
        juce::dsp::Oversampling<float> oversampler;
        double                          sampleRate { 0.0 };
        // Scratch buffers sized to the largest chunk processBlock() ever
        // hands the oversampler in one call -- see OversampledDistortion.h
        // for why these are sized to kMaxOversampledBlockSize rather than
        // one sample.
        juce::AudioBuffer<float>       osIn  { 1, kMaxOversampledBlockSize };
        juce::AudioBuffer<float>       osOut { 1, kMaxOversampledBlockSize };
        bool                           prepared            { false };
        bool                           oversamplingEnabled { true };
    };
}

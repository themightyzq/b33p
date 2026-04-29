#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace B33p
{
    // Wet-effect slot that sits at the end of the per-voice signal
    // chain, after distortion. One mode is active at a time (chosen
    // via setType); the others sit idle. State lives on this object
    // for every supported mode so a type switch mid-pattern doesn't
    // need to re-prepare anything.
    //
    // The three normalised parameters (p1, p2, mix) carry
    // type-dependent semantics so the existing two-knob-plus-mix UI
    // can drive every mode without per-mode parameter sets:
    //
    //   Chorus  : p1 = rate (0.1..5 Hz), p2 = depth, mix = wet/dry
    //   Reverb  : p1 = roomSize, p2 = damping, mix = wet level
    //   Delay   : p1 = time (1..2000 ms, log-skewed), p2 = feedback
    //             (0..0.95), mix = wet/dry
    //   Flanger : p1 = rate (0.1..3 Hz), p2 = depth, mix = wet/dry
    //             (feedback fixed at 0.7 for the metallic flanger
    //             character)
    //   Phaser  : p1 = rate (0.1..5 Hz), p2 = depth, mix = wet/dry
    //
    // Lifecycle: construct -> prepare(sampleRate) -> setType /
    // setParamN / setMix in any order -> processSample. Before
    // prepare(), processSample returns the input unchanged (an
    // identity bypass). reset() returns delay buffers and modulator
    // phase to a fresh state.
    class ModulationEffect
    {
    public:
        enum class Type
        {
            None,
            Chorus,
            Reverb,
            Delay,
            Flanger,
            Phaser
        };

        void prepare(double sampleRate);
        void reset();

        void setType(Type type);
        void setParam1(float v01);
        void setParam2(float v01);
        void setMix(float v01);

        float processSample(float input);

    private:
        void   pushParamsToActiveType();
        float  delaySamplesFromP1() const;
        float  feedbackFromP2() const;

        Type   type       { Type::None };
        float  p1         { 0.5f };
        float  p2         { 0.5f };
        float  mix        { 0.5f };
        double sampleRate { 0.0 };
        bool   prepared   { false };

        // JUCE dsp processors that don't expose sample-at-a-time
        // APIs are driven via 1-sample AudioBlocks per call. The
        // overhead is acceptable inside our per-voice loop because
        // there are only four voices.
        juce::dsp::Chorus<float>  chorus;
        juce::dsp::Chorus<float>  flanger;
        juce::dsp::Phaser<float>  phaser;
        juce::dsp::DelayLine<float,
            juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;

        // juce::Reverb (not in the dsp namespace) has the simplest
        // sample-at-a-time interface — processMono on a 1-sample
        // buffer. Plenty for our purposes.
        juce::Reverb              reverb;
        juce::Reverb::Parameters  reverbParams;
    };
}

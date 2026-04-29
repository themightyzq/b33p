#include "ModulationEffect.h"

#include <algorithm>
#include <cmath>

namespace B33p
{
    namespace
    {
        constexpr float kMinDelayMs = 1.0f;
        constexpr float kMaxDelayMs = 2000.0f;
    }

    void ModulationEffect::prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        prepared   = true;

        // 1-sample, 1-channel ProcessSpec — the JUCE dsp processors
        // are exclusively used in sample-at-a-time mode here.
        const juce::dsp::ProcessSpec spec {
            sampleRate, 1u, 1u
        };
        chorus.prepare(spec);
        flanger.prepare(spec);
        phaser.prepare(spec);
        delayLine.prepare(spec);
        delayLine.setMaximumDelayInSamples(
            static_cast<int>(sampleRate * (kMaxDelayMs / 1000.0)));

        reverb.setSampleRate(sampleRate);

        // Push current parameters into whichever effect is active —
        // setters might have been called before prepare().
        pushParamsToActiveType();
    }

    void ModulationEffect::reset()
    {
        chorus.reset();
        flanger.reset();
        phaser.reset();
        delayLine.reset();
        reverb.reset();
    }

    void ModulationEffect::setType(Type newType)
    {
        if (type == newType)
            return;
        type = newType;
        // Clean any modulator phase / delay-line memory left from
        // the previous type before re-pushing parameters. Otherwise
        // a type flip mid-event can leak the old type's tail.
        reset();
        pushParamsToActiveType();
    }

    void ModulationEffect::setParam1(float v01)
    {
        p1 = std::clamp(v01, 0.0f, 1.0f);
        pushParamsToActiveType();
    }

    void ModulationEffect::setParam2(float v01)
    {
        p2 = std::clamp(v01, 0.0f, 1.0f);
        pushParamsToActiveType();
    }

    void ModulationEffect::setMix(float v01)
    {
        mix = std::clamp(v01, 0.0f, 1.0f);
        pushParamsToActiveType();
    }

    float ModulationEffect::delaySamplesFromP1() const
    {
        // Log-skew the 1..2000 ms range so the slider's lower half
        // covers the more useful short-delay region.
        const float t   = std::clamp(p1, 0.0f, 1.0f);
        const float ms  = kMinDelayMs * std::pow(kMaxDelayMs / kMinDelayMs, t);
        return static_cast<float>(sampleRate) * (ms / 1000.0f);
    }

    float ModulationEffect::feedbackFromP2() const
    {
        // Cap at 0.95 so a sustained input can't run the delay's
        // feedback loop into clipping / runaway oscillation.
        return std::clamp(p2, 0.0f, 1.0f) * 0.95f;
    }

    void ModulationEffect::pushParamsToActiveType()
    {
        if (! prepared)
            return;

        switch (type)
        {
            case Type::None:
                break;

            case Type::Chorus:
                chorus.setRate       (0.1f + p1 * 4.9f);   // 0.1..5 Hz
                chorus.setDepth      (p2);                   // 0..1
                chorus.setCentreDelay(15.0f);                // ms — chorus territory
                chorus.setFeedback   (0.0f);                 // chorus is feedback-free
                chorus.setMix        (mix);
                break;

            case Type::Reverb:
                reverbParams.roomSize   = p1;
                reverbParams.damping    = p2;
                reverbParams.wetLevel   = mix;
                reverbParams.dryLevel   = 1.0f - mix;
                reverbParams.width      = 1.0f;
                reverbParams.freezeMode = 0.0f;
                reverb.setParameters(reverbParams);
                break;

            case Type::Delay:
                delayLine.setDelay(juce::jlimit(1.0f,
                                                 static_cast<float>(delayLine.getMaximumDelayInSamples()),
                                                 delaySamplesFromP1()));
                break;

            case Type::Flanger:
                flanger.setRate       (0.1f + p1 * 2.9f);   // 0.1..3 Hz
                flanger.setDepth      (p2);
                flanger.setCentreDelay(2.0f);                // ms — flanger territory
                flanger.setFeedback   (0.7f);                // fixed metallic feedback
                flanger.setMix        (mix);
                break;

            case Type::Phaser:
                phaser.setRate          (0.1f + p1 * 4.9f); // 0.1..5 Hz
                phaser.setDepth         (p2);
                phaser.setCentreFrequency(1000.0f);          // Hz — mid-range sweep
                phaser.setFeedback      (0.5f);              // moderate
                phaser.setMix           (mix);
                break;
        }
    }

    float ModulationEffect::processSample(float input)
    {
        if (! prepared)
            return input;

        switch (type)
        {
            case Type::None:
                return input;

            case Type::Chorus:
            {
                float sample = input;
                float* ptrs[1] = { &sample };
                juce::dsp::AudioBlock<float> block(ptrs, 1, 1);
                juce::dsp::ProcessContextReplacing<float> ctx(block);
                chorus.process(ctx);
                return sample;
            }

            case Type::Reverb:
            {
                float sample = input;
                reverb.processMono(&sample, 1);
                return sample;
            }

            case Type::Delay:
            {
                // Manual feedback loop: read N samples back, mix in
                // the dry input scaled by feedback, push the sum
                // back, and return a wet/dry blend.
                const float feedback = feedbackFromP2();
                const float pop      = delayLine.popSample(0);
                delayLine.pushSample(0, input + pop * feedback);
                return (1.0f - mix) * input + mix * pop;
            }

            case Type::Flanger:
            {
                float sample = input;
                float* ptrs[1] = { &sample };
                juce::dsp::AudioBlock<float> block(ptrs, 1, 1);
                juce::dsp::ProcessContextReplacing<float> ctx(block);
                flanger.process(ctx);
                return sample;
            }

            case Type::Phaser:
            {
                float sample = input;
                float* ptrs[1] = { &sample };
                juce::dsp::AudioBlock<float> block(ptrs, 1, 1);
                juce::dsp::ProcessContextReplacing<float> ctx(block);
                phaser.process(ctx);
                return sample;
            }
        }
        return input;
    }
}

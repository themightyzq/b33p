#include <catch2/catch_test_macros.hpp>

#include "Core/ParameterIDs.h"
#include "DSP/Filter.h"
#include "DSP/ModulationEffect.h"
#include "DSP/Oscillator.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

using B33p::B33pProcessor;
using B33p::Event;
using B33p::Oscillator;

namespace IDs = B33p::ParameterIDs;

// Voice skips the oversampling filters of its bitcrush + distortion stages
// once a voice has gone silent (Voice::isEffectsTailIdle). That skip must not
// change the output by a single bit: these tests render the same session
// with the skip on and off and compare.

namespace
{
    constexpr double kSampleRate = 48000.0;

    void setReal(B33pProcessor& p, const juce::String& id, float realValue)
    {
        auto* param = p.getApvts().getParameter(id);
        REQUIRE(param != nullptr);
        const float norm = param->getNormalisableRange().convertTo0to1(realValue);
        param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, norm));
    }

    void configureLane(B33pProcessor& p, int lane, Oscillator::Waveform wf,
                       B33p::ModulationEffect::Type fx)
    {
        setReal(p, IDs::oscWaveform(lane),        static_cast<float>(wf));
        setReal(p, IDs::basePitchHz(lane),        260.0f + 70.0f * static_cast<float>(lane));
        setReal(p, IDs::ampAttack(lane),          0.002f);
        setReal(p, IDs::ampDecay(lane),           0.05f);
        setReal(p, IDs::ampSustain(lane),         0.7f);
        setReal(p, IDs::ampRelease(lane),         0.25f);   // a real release tail
        setReal(p, IDs::filterType(lane),         static_cast<float>(B33p::Filter::Type::Lowpass));
        setReal(p, IDs::filterCutoffHz(lane),     3000.0f);
        setReal(p, IDs::distortionDrive(lane),    3.0f);
        setReal(p, IDs::modEffectType(lane),      static_cast<float>(fx));
        setReal(p, IDs::modEffectParam1(lane),    0.45f);
        setReal(p, IDs::modEffectParam2(lane),    0.6f);
        setReal(p, IDs::modEffectMix(lane),       0.5f);
        setReal(p, IDs::voiceGain(lane),          0.4f);
    }

    // Four lanes covering every stage the skip touches or must leave alone:
    // a reverb tail, a delay tail with feedback, a low-rate bitcrush (long
    // sample-and-hold period) into a chorus, and noise into a phaser. Events
    // sit in the first 0.4 s of a 1.6 s loop, so each voice goes idle for
    // over a second and is then retriggered when the loop comes round.
    std::unique_ptr<B33pProcessor> makeSession(bool skipEnabled)
    {
        using W  = Oscillator::Waveform;
        using FX = B33p::ModulationEffect::Type;

        auto p = std::make_unique<B33pProcessor>();
        configureLane(*p, 0, W::Sine,   FX::Reverb);
        configureLane(*p, 1, W::Square, FX::Delay);
        configureLane(*p, 2, W::Saw,    FX::Chorus);
        configureLane(*p, 3, W::Noise,  FX::Phaser);
        setReal(*p, IDs::bitcrushBitDepth(2),     5.0f);
        setReal(*p, IDs::bitcrushSampleRateHz(2), 150.0f);
        setReal(*p, IDs::bitcrushBitDepth(3),     9.0f);
        setReal(*p, IDs::bitcrushSampleRateHz(3), 2500.0f);

        auto& pattern = p->getPattern();
        pattern.setLengthSeconds(1.6);
        for (int lane = 0; lane < B33p::Pattern::kNumLanes; ++lane)
        {
            Event e;
            e.startSeconds    = 0.05 + 0.07 * lane;
            e.durationSeconds = 0.12;
            pattern.addEvent(lane, e);
            Event ratchet;
            ratchet.startSeconds    = 0.25 + 0.03 * lane;
            ratchet.durationSeconds = 0.1;
            ratchet.ratchets        = 2;
            ratchet.pitchOffsetSemitones = 5.0f;
            pattern.addEvent(lane, ratchet);
        }

        p->setLooping(true);
        p->setIdleSkipEnabledForTests(skipEnabled);
        p->seedRandomSources(77);
        return p;
    }

    struct Render
    {
        std::vector<float> samples;
        int                maxIdleVoices { 0 };
    };

    // ~4 s: two and a half loops, MIDI notes on the pooled voices, and
    // parameter changes made while voices are idle (the next trigger must
    // meet the same smoother and sample-and-hold state either way).
    Render render(B33pProcessor& p, int blockSize)
    {
        p.prepareToPlay(kSampleRate, blockSize);
        p.startPlayback();

        Render r;
        const int total = static_cast<int>(4.0 * kSampleRate);
        juce::AudioBuffer<float> block(2, blockSize);
        juce::MidiBuffer midi;
        bool changedWhileIdle = false;
        for (int start = 0; start < total; start += blockSize)
        {
            const int n = std::min(blockSize, total - start);
            juce::AudioBuffer<float> view(block.getArrayOfWritePointers(), 2, n);
            view.clear();
            midi.clear();

            const double t = static_cast<double>(start) / kSampleRate;
            if (t >= 0.3 && t < 0.3 + static_cast<double>(n) / kSampleRate)
                midi.addEvent(juce::MidiMessage::noteOn(1, 62, 0.7f), n / 3);
            if (t >= 0.5 && t < 0.5 + static_cast<double>(n) / kSampleRate)
                midi.addEvent(juce::MidiMessage::noteOff(1, 62), n / 2);
            if (t >= 2.0 && t < 2.0 + static_cast<double>(n) / kSampleRate)
                midi.addEvent(juce::MidiMessage::noteOn(1, 70, 0.9f), 0);

            if (! changedWhileIdle && t >= 1.3)
            {
                changedWhileIdle = true;
                setReal(p, IDs::bitcrushSampleRateHz(2), 400.0f);
                setReal(p, IDs::bitcrushBitDepth(2),     7.0f);
                setReal(p, IDs::distortionDrive(1),      8.0f);
                setReal(p, IDs::modEffectMix(0),         0.8f);
                setReal(p, IDs::voiceGain(3),            0.6f);
            }

            p.processBlock(view, midi);
            for (int i = 0; i < n; ++i)
                r.samples.push_back(view.getSample(0, i));
            r.maxIdleVoices = std::max(r.maxIdleVoices, p.getNumIdleVoicesForTests());
        }
        return r;
    }
}

TEST_CASE("Idle voices skipping their oversamplers leaves the output bit-identical, "
          "including release, reverb and delay tails and retriggers after idle",
          "[state][idle]")
{
    for (int blockSize : { 256, 100, 2048 })
    {
        INFO("block size " << blockSize);
        auto withSkip    = makeSession(true);
        auto withoutSkip = makeSession(false);
        const auto a = render(*withSkip,    blockSize);
        const auto b = render(*withoutSkip, blockSize);

        // The skip really engaged (otherwise this test proves nothing):
        // all 8 MIDI-pool voices, and the lanes between notes.
        REQUIRE(a.maxIdleVoices >= 8 + 2);
        REQUIRE(b.maxIdleVoices == 0);

        REQUIRE(a.samples.size() == b.samples.size());
        size_t firstDiff = a.samples.size();
        for (size_t i = 0; i < a.samples.size(); ++i)
            if (! juce::exactlyEqual(a.samples[i], b.samples[i]))
            {
                firstDiff = i;
                break;
            }
        INFO("first differing sample " << firstDiff);
        REQUIRE(firstDiff == a.samples.size());

        // And the render is not silence.
        float peak = 0.0f;
        for (float s : a.samples)
            peak = std::max(peak, std::fabs(s));
        REQUIRE(peak > 0.05f);
    }
}

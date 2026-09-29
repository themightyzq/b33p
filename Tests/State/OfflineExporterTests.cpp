#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Core/ParameterIDs.h"
#include "DSP/Filter.h"
#include "DSP/ModulationEffect.h"
#include "DSP/ModulationMatrix.h"
#include "DSP/Oscillator.h"
#include "State/B33pProcessor.h"
#include "State/OfflineExporter.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <cmath>
#include <map>
#include <memory>
#include <vector>

using B33p::B33pProcessor;
using B33p::Event;
using B33p::ModDestination;
using B33p::ModSource;
using B33p::OfflineExporter;
using B33p::Oscillator;
using B33p::Pattern;
using Catch::Approx;

namespace IDs = B33p::ParameterIDs;

namespace
{
    constexpr double kSampleRate = 48000.0;

    // Writes a real-world value (Hz, seconds, choice index) through the
    // parameter's normalised range, the way a host or the UI would.
    void setReal(B33pProcessor& p, const juce::String& id, float realValue)
    {
        auto* param = p.getApvts().getParameter(id);
        REQUIRE(param != nullptr);
        const float norm = param->getNormalisableRange().convertTo0to1(realValue);
        param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, norm));
    }

    void setChoice(B33pProcessor& p, const juce::String& id, int index)
    {
        setReal(p, id, static_cast<float>(index));
    }

    // Four distinct single-cycle tables with values that do not survive
    // a lossy float -> text -> float round trip by accident.
    void loadDistinctWavetables(B33pProcessor& p, int lane)
    {
        for (int slot = 0; slot < Oscillator::kNumWavetableSlots; ++slot)
        {
            std::vector<float> table(static_cast<size_t>(Oscillator::kCustomTableSize));
            for (size_t i = 0; i < table.size(); ++i)
            {
                const double ph = static_cast<double>(i) / static_cast<double>(table.size());
                const double sine = std::sin(2.0 * juce::MathConstants<double>::pi * ph
                                             * static_cast<double>(slot + 1));
                const double saw  = 2.0 * ph - 1.0;
                table[i] = static_cast<float>(0.7123457 * sine + 0.1987654 * slot * saw / 3.0);
            }
            p.setWavetableSlot(lane, slot, std::move(table));
        }
    }

    // A lane that exercises everything the old reduced export model
    // dropped: the given waveform, a non-default filter type, FM / ring /
    // wavetable settings, a Mod FX, and an LFO routed through the matrix.
    void configureLane(B33pProcessor& p, int lane, Oscillator::Waveform waveform,
                       B33p::Filter::Type filterType,
                       B33p::ModulationEffect::Type modFx)
    {
        setChoice(p, IDs::oscWaveform(lane), static_cast<int>(waveform));
        setReal  (p, IDs::basePitchHz(lane),      330.0f);
        setReal  (p, IDs::wavetableMorph(lane),   0.37f);
        setReal  (p, IDs::fmRatio(lane),          2.0f);
        setReal  (p, IDs::fmDepth(lane),          3.0f);
        setReal  (p, IDs::ringRatio(lane),        1.5f);
        setReal  (p, IDs::ringMix(lane),          0.7f);
        setReal  (p, IDs::ampAttack(lane),        0.002f);
        setReal  (p, IDs::ampDecay(lane),         0.05f);
        setReal  (p, IDs::ampSustain(lane),       0.6f);
        setReal  (p, IDs::ampRelease(lane),       0.08f);
        setChoice(p, IDs::filterType(lane),       static_cast<int>(filterType));
        setReal  (p, IDs::filterCutoffHz(lane),   1800.0f);
        setReal  (p, IDs::filterResonanceQ(lane), 1.5f);
        setReal  (p, IDs::filterVowel(lane),      0.4f);
        setReal  (p, IDs::distortionDrive(lane),  2.0f);
        setChoice(p, IDs::modEffectType(lane),    static_cast<int>(modFx));
        setReal  (p, IDs::modEffectParam1(lane),  0.4f);
        setReal  (p, IDs::modEffectParam2(lane),  0.5f);
        setReal  (p, IDs::modEffectMix(lane),     0.5f);
        setReal  (p, IDs::voiceGain(lane),        0.4f);

        setReal  (p, IDs::lfoRateHz(lane, 0),     5.0f);
        setChoice(p, IDs::modSlotSource(lane, 0), static_cast<int>(ModSource::LFO1));
        setChoice(p, IDs::modSlotDest(lane, 0),   static_cast<int>(ModDestination::FilterCutoff));
        setReal  (p, IDs::modSlotAmount(lane, 0), 0.4f);

        loadDistinctWavetables(p, lane);
    }

    // Events that use probability, ratchets, humanize and a per-event
    // override, so the seeded snapshot RNG and the override path are
    // part of what has to match.
    void addTestPattern(B33pProcessor& p, int lane)
    {
        auto& pattern = p.getPattern();
        pattern.setLengthSeconds(0.5);

        Event a;
        a.startSeconds = 0.0;
        a.durationSeconds = 0.1;
        pattern.addEvent(lane, a);

        Event b;
        b.startSeconds = 0.15;
        b.durationSeconds = 0.12;
        b.pitchOffsetSemitones = 7.0f;
        b.velocity = 0.8f;
        b.probability = 0.6f;
        b.ratchets = 3;
        b.humanizeAmount = 0.4f;
        pattern.addEvent(lane, b);

        Event c;
        c.startSeconds = 0.32;
        c.durationSeconds = 0.1;
        c.pitchOffsetSemitones = -5.0f;
        c.overrides[0] = { ModDestination::FilterCutoff, 0.25f };
        pattern.addEvent(lane, c);
    }

    // "Playback": drive the given processor's own processBlock with the
    // same seed and block schedule the exporter uses, then drop the
    // reported latency. This is what a host sees when it plays the
    // pattern once, non-looped.
    juce::AudioBuffer<float> playLive(B33pProcessor& live,
                                      const OfflineExporter::Settings& s,
                                      int numOutSamples)
    {
        live.setLooping(false);
        live.setFollowHostTransport(false);
        live.seedRandomSources(s.seed);
        live.prepareToPlay(s.sampleRate, s.blockSize);
        const int latency = live.getLatencySamples();
        live.startPlayback();

        const int needed = latency + numOutSamples;
        juce::AudioBuffer<float> all(1, needed + s.blockSize);
        juce::AudioBuffer<float> block(2, s.blockSize);
        juce::MidiBuffer         midi;
        for (int written = 0; written < needed; written += s.blockSize)
        {
            block.clear();
            midi.clear();
            live.processBlock(block, midi);
            all.copyFrom(0, written, block, 0, 0, s.blockSize);
        }

        juce::AudioBuffer<float> out(1, numOutSamples);
        out.copyFrom(0, 0, all, 0, latency, numOutSamples);
        return out;
    }

    juce::AudioBuffer<float> exportFrom(B33pProcessor& live,
                                        const OfflineExporter::Settings& s)
    {
        OfflineExporter exporter(OfflineExporter::captureState(live));
        return exporter.render(s);
    }

    float peakAbs(const juce::AudioBuffer<float>& b)
    {
        float peak = 0.0f;
        for (int i = 0; i < b.getNumSamples(); ++i)
            peak = std::max(peak, std::fabs(b.getSample(0, i)));
        return peak;
    }

    float maxAbsDiff(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        REQUIRE(a.getNumSamples() == b.getNumSamples());
        float d = 0.0f;
        for (int i = 0; i < a.getNumSamples(); ++i)
            d = std::max(d, std::fabs(a.getSample(0, i) - b.getSample(0, i)));
        return d;
    }

    int lastIndexAbove(const juce::AudioBuffer<float>& b, float threshold)
    {
        for (int i = b.getNumSamples() - 1; i >= 0; --i)
            if (std::fabs(b.getSample(0, i)) > threshold)
                return i;
        return -1;
    }

    int firstIndexAbove(const juce::AudioBuffer<float>& b, float threshold)
    {
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (std::fabs(b.getSample(0, i)) > threshold)
                return i;
        return -1;
    }

    std::map<juce::String, float> allParameterValues(B33pProcessor& p)
    {
        std::map<juce::String, float> values;
        for (auto* param : p.getParameters())
            if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
                values[withId->paramID] = withId->getValue();
        return values;
    }

    // Lists "id: a vs b" for every parameter whose normalised value
    // differs by more than `tolerance`, so a failed comparison names the
    // parameter instead of printing {?}.
    juce::String describeDifferences(const std::map<juce::String, float>& a,
                                     const std::map<juce::String, float>& b,
                                     float tolerance = 0.0f)
    {
        juce::String out;
        for (const auto& [id, value] : a)
        {
            const auto it = b.find(id);
            if (it == b.end())
                out << id << ": missing\n";
            else if (std::fabs(it->second - value) > tolerance)
                out << id << ": " << value << " vs " << it->second << "\n";
        }
        return out;
    }

    // A bare sine lane with instant attack, used by the simpler
    // behavioural tests (ported from the old PatternRenderer suite).
    void configurePlainSine(B33pProcessor& p, float gain, float releaseSeconds)
    {
        setChoice(p, IDs::oscWaveform(0), static_cast<int>(Oscillator::Waveform::Sine));
        setReal  (p, IDs::basePitchHz(0), 440.0f);
        setReal  (p, IDs::ampAttack(0),   0.0f);
        setReal  (p, IDs::ampDecay(0),    0.0f);
        setReal  (p, IDs::ampSustain(0),  1.0f);
        setReal  (p, IDs::ampRelease(0),  releaseSeconds);
        setReal  (p, IDs::voiceGain(0),   gain);
    }
}

TEST_CASE("OfflineExporter: export equals playback for every waveform, "
          "with a non-default filter, a Mod FX and an LFO route",
          "[state][export]")
{
    struct Case
    {
        Oscillator::Waveform           waveform;
        B33p::Filter::Type             filter;
        B33p::ModulationEffect::Type   modFx;
    };
    using W  = Oscillator::Waveform;
    using F  = B33p::Filter::Type;
    using FX = B33p::ModulationEffect::Type;
    const Case cases[] = {
        { W::Sine,      F::Bandpass, FX::Chorus  },
        { W::Square,    F::Highpass, FX::Reverb  },
        { W::Triangle,  F::Comb,     FX::Delay   },
        { W::Saw,       F::Formant,  FX::Flanger },
        { W::Noise,     F::Bandpass, FX::Phaser  },
        { W::Custom,    F::Highpass, FX::Chorus  },
        { W::Wavetable, F::Formant,  FX::Reverb  },
        { W::FM,        F::Comb,     FX::Flanger },
        { W::Ring,      F::Bandpass, FX::Delay   },
    };

    for (const auto& c : cases)
    {
        INFO("waveform index " << static_cast<int>(c.waveform));

        auto live = std::make_unique<B33pProcessor>();
        configureLane(*live, 0, c.waveform, c.filter, c.modFx);
        // A second lane on a different setup so the mix is exercised.
        configureLane(*live, 2, W::Saw, F::Lowpass, FX::None);
        addTestPattern(*live, 0);
        live->getPattern().addEvent(2, Event { 0.05, 0.2, 12.0f });

        OfflineExporter::Settings s;
        s.sampleRate = kSampleRate;

        const auto exported = exportFrom(*live, s);
        REQUIRE(exported.getNumSamples() >= juce::roundToInt(0.5 * kSampleRate));
        REQUIRE(peakAbs(exported) > 0.01f);

        const auto played = playLive(*live, s, exported.getNumSamples());
        REQUIRE(maxAbsDiff(exported, played) < 1.0e-4f);
    }
}

TEST_CASE("OfflineExporter: equals playback at a non-power-of-two block size and 96 kHz",
          "[state][export]")
{
    auto live = std::make_unique<B33pProcessor>();
    configureLane(*live, 0, Oscillator::Waveform::Wavetable,
                  B33p::Filter::Type::Bandpass, B33p::ModulationEffect::Type::Chorus);
    addTestPattern(*live, 0);

    OfflineExporter::Settings s;
    s.sampleRate = 96000.0;
    s.blockSize  = 100;

    const auto exported = exportFrom(*live, s);
    REQUIRE(peakAbs(exported) > 0.01f);
    const auto played = playLive(*live, s, exported.getNumSamples());
    REQUIRE(maxAbsDiff(exported, played) < 1.0e-4f);
}

TEST_CASE("OfflineExporter: a render is reproducible, and the seed drives Noise",
          "[state][export]")
{
    auto live = std::make_unique<B33pProcessor>();
    configureLane(*live, 0, Oscillator::Waveform::Noise,
                  B33p::Filter::Type::Bandpass, B33p::ModulationEffect::Type::None);
    addTestPattern(*live, 0);

    OfflineExporter::Settings s;
    s.sampleRate = kSampleRate;

    OfflineExporter exporter(OfflineExporter::captureState(*live));
    const auto first  = exporter.render(s);
    const auto second = exporter.render(s);   // same instance, re-prepared
    const auto fresh  = exportFrom(*live, s);  // separate instance
    REQUIRE(maxAbsDiff(first, second) == 0.0f);
    REQUIRE(maxAbsDiff(first, fresh)  == 0.0f);

    auto other = s;
    other.seed = s.seed + 1;
    const auto reseeded = exporter.render(other);
    const int n = std::min(first.getNumSamples(), reseeded.getNumSamples());
    float diff = 0.0f;
    for (int i = 0; i < n; ++i)
        diff = std::max(diff, std::fabs(first.getSample(0, i) - reseeded.getSample(0, i)));
    REQUIRE(diff > 1.0e-3f);
}

TEST_CASE("OfflineExporter: loop, follow-host and bypass in the saved state do not silence the export",
          "[state][export]")
{
    auto live = std::make_unique<B33pProcessor>();
    configurePlainSine(*live, 0.5f, 0.05f);
    live->getPattern().setLengthSeconds(0.3);
    live->getPattern().addEvent(0, Event { 0.0, 0.1, 0.0f });

    OfflineExporter::Settings s;
    s.sampleRate = kSampleRate;
    const auto plain = exportFrom(*live, s);

    live->setLooping(true);
    live->setFollowHostTransport(true);
    live->getApvts().getParameter(IDs::bypass())->setValueNotifyingHost(1.0f);
    const auto flagged = exportFrom(*live, s);

    REQUIRE(peakAbs(flagged) > 0.1f);
    REQUIRE(maxAbsDiff(plain, flagged) == 0.0f);
}

TEST_CASE("OfflineExporter: the oversampling latency is trimmed so a hit at 0 starts the file",
          "[state][export]")
{
    auto live = std::make_unique<B33pProcessor>();
    configurePlainSine(*live, 0.5f, 0.05f);
    setChoice(*live, IDs::oscWaveform(0), static_cast<int>(Oscillator::Waveform::Square));
    live->getPattern().setLengthSeconds(0.2);
    live->getPattern().addEvent(0, Event { 0.0, 0.1, 0.0f });

    OfflineExporter::Settings s;
    s.sampleRate = kSampleRate;
    const auto exported = exportFrom(*live, s);

    B33pProcessor probe;
    probe.prepareToPlay(kSampleRate, s.blockSize);
    const int latency = probe.getLatencySamples();
    REQUIRE(latency > 0);

    // Untrimmed, the first hit would surface around sample `latency`.
    const int onset = firstIndexAbove(exported, 0.05f);
    REQUIRE(onset >= 0);
    REQUIRE(onset < latency / 2);
}

TEST_CASE("OfflineExporter: empty pattern renders silence of exactly the pattern length",
          "[state][export]")
{
    B33pProcessor live;
    live.getPattern().setLengthSeconds(0.5);

    OfflineExporter::Settings s;
    s.sampleRate = kSampleRate;
    const auto buf = exportFrom(live, s);

    REQUIRE(buf.getNumChannels() == 1);
    REQUIRE(buf.getNumSamples() == juce::roundToInt(0.5 * kSampleRate));
    REQUIRE(peakAbs(buf) == Approx(0.0f).margin(1e-6f));
}

TEST_CASE("OfflineExporter: a single event is audible and the file stays bounded",
          "[state][export]")
{
    B33pProcessor live;
    configurePlainSine(live, 0.5f, 0.05f);
    live.getPattern().setLengthSeconds(0.5);
    live.getPattern().addEvent(0, Event { 0.1, 0.2, 0.0f });

    OfflineExporter::Settings s;
    s.sampleRate = kSampleRate;
    const auto buf = exportFrom(live, s);

    REQUIRE(buf.getNumSamples() >= juce::roundToInt(0.5 * kSampleRate));
    REQUIRE(buf.getNumSamples() <= juce::roundToInt(1.5 * kSampleRate));
    REQUIRE(peakAbs(buf) > 0.05f);
}

TEST_CASE("OfflineExporter: a release tail extends past the pattern end",
          "[state][export]")
{
    B33pProcessor live;
    configurePlainSine(live, 0.5f, 0.1f);
    live.getPattern().setLengthSeconds(0.2);
    live.getPattern().addEvent(0, Event { 0.18, 0.05, 0.0f });

    OfflineExporter::Settings s;
    s.sampleRate = kSampleRate;
    const auto buf = exportFrom(live, s);

    const int patternSamples = juce::roundToInt(0.2 * kSampleRate);
    REQUIRE(buf.getNumSamples() > patternSamples);
    REQUIRE(lastIndexAbove(buf, 1.0e-3f) > patternSamples);
}

TEST_CASE("OfflineExporter: maxTailSeconds caps the tail",
          "[state][export]")
{
    B33pProcessor live;
    configurePlainSine(live, 0.5f, 5.0f);   // would ring for ~5 s
    live.getPattern().setLengthSeconds(0.1);
    live.getPattern().addEvent(0, Event { 0.0, 0.05, 0.0f });

    OfflineExporter::Settings s;
    s.sampleRate     = kSampleRate;
    s.maxTailSeconds = 0.2;
    const auto buf = exportFrom(live, s);

    const int maxAllowed = juce::roundToInt(0.1 * kSampleRate) + juce::roundToInt(0.2 * kSampleRate);
    REQUIRE(buf.getNumSamples() <= maxAllowed);
}

TEST_CASE("OfflineExporter: a Delay echo after a silent gap is kept",
          "[state][export]")
{
    // Delay time ~1.8 s: the echo arrives after well over a second of
    // silence, which a per-block "is it quiet yet" check would cut.
    B33pProcessor live;
    configurePlainSine(live, 0.5f, 0.02f);
    setChoice(live, IDs::modEffectType(0), static_cast<int>(B33p::ModulationEffect::Type::Delay));
    setReal  (live, IDs::modEffectParam1(0), 0.986f);
    setReal  (live, IDs::modEffectParam2(0), 0.2f);
    setReal  (live, IDs::modEffectMix(0),    0.5f);
    live.getPattern().setLengthSeconds(0.2);
    live.getPattern().addEvent(0, Event { 0.0, 0.05, 0.0f });

    OfflineExporter::Settings s;
    s.sampleRate = kSampleRate;
    const auto buf = exportFrom(live, s);

    REQUIRE(lastIndexAbove(buf, 1.0e-3f) > juce::roundToInt(1.7 * kSampleRate));
}

TEST_CASE("OfflineExporter: rendered level follows the voice gain",
          "[state][export]")
{
    // Both gains keep the peak under the output limiter's knee, so the
    // ratio is exact apart from smoothing.
    B33pProcessor half;
    configurePlainSine(half, 0.5f, 0.05f);
    half.getPattern().setLengthSeconds(0.2);
    half.getPattern().addEvent(0, Event { 0.0, 0.1, 0.0f });

    B33pProcessor quarter;
    configurePlainSine(quarter, 0.25f, 0.05f);
    quarter.getPattern().setLengthSeconds(0.2);
    quarter.getPattern().addEvent(0, Event { 0.0, 0.1, 0.0f });

    OfflineExporter::Settings s;
    s.sampleRate = kSampleRate;
    const float peakHalf    = peakAbs(exportFrom(half, s));
    const float peakQuarter = peakAbs(exportFrom(quarter, s));

    REQUIRE(peakHalf > 0.05f);
    REQUIRE(peakQuarter == Approx(peakHalf * 0.5f).margin(0.02f));
}

TEST_CASE("OfflineExporter: batch export rolls its own copy and never changes the live processor",
          "[state][export]")
{
    auto live = std::make_unique<B33pProcessor>();
    configureLane(*live, 0, Oscillator::Waveform::Saw,
                  B33p::Filter::Type::Lowpass, B33p::ModulationEffect::Type::None);
    addTestPattern(*live, 0);
    live->getRandomizer().setLocked(IDs::filterCutoffHz(0), true);

    const auto before = allParameterValues(*live);

    OfflineExporter::Settings s;
    s.sampleRate = kSampleRate;

    OfflineExporter exporter(OfflineExporter::captureState(*live));
    juce::Random rollRng(1234);

    const auto variation0 = exporter.renderVariation(0, s, rollRng);
    // The state copy goes through the session serialiser; a skewed
    // parameter can come back one float ULP off, as on a DAW reload.
    const auto afterLoad = describeDifferences(before, allParameterValues(exporter.getProcessor()),
                                               1.0e-6f);
    INFO(afterLoad);
    REQUIRE(afterLoad.isEmpty());
    REQUIRE(maxAbsDiff(variation0, exportFrom(*live, s)) == 0.0f);

    for (int i = 1; i < 3; ++i)
        (void) exporter.renderVariation(i, s, rollRng);

    // The live processor is untouched...
    const auto liveChanges = describeDifferences(before, allParameterValues(*live));
    INFO(liveChanges);
    REQUIRE(liveChanges.isEmpty());

    // ...while the exporter's copy really was rolled, with the lock
    // carried over from the live state.
    const auto rolled = allParameterValues(exporter.getProcessor());
    REQUIRE(rolled != before);
    REQUIRE(rolled.at(IDs::filterCutoffHz(0))
            == Approx(before.at(IDs::filterCutoffHz(0))).margin(1.0e-6f));
}

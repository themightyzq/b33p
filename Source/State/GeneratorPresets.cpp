#include "GeneratorPresets.h"

#include "B33pProcessor.h"
#include "Core/ParameterIDs.h"
#include "DSP/Filter.h"
#include "DSP/ModulationEffect.h"
#include "DSP/Oscillator.h"

namespace B33p
{
    namespace
    {
        // Sets an APVTS parameter to its desired natural-range
        // value. Looks up the parameter by ID, converts to the
        // 0..1 normalised space the API speaks, and notifies via
        // setValueNotifyingHost so any listeners (and the dirty
        // flag) catch the change. No-op if the ID doesn't resolve.
        void setParam(B33pProcessor& processor, const juce::String& id, float value)
        {
            auto& apvts = processor.getApvts();
            if (auto* p = apvts.getParameter(id))
                p->setValueNotifyingHost(p->convertTo0to1(value));
        }

        void setChoice(B33pProcessor& processor, const juce::String& id, int index)
        {
            auto& apvts = processor.getApvts();
            if (auto* p = apvts.getParameter(id))
                p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(index)));
        }

        // Cycles through a few notes inside the pattern length.
        // count events evenly spaced from start to end with
        // duration = (end - start) / count.
        void addEvenlySpacedEvents(B33pProcessor& processor,
                                    int lane,
                                    int count,
                                    double startSeconds,
                                    double endSeconds,
                                    float velocity = 1.0f,
                                    float pitchOffsetSemitones = 0.0f)
        {
            if (count <= 0)
                return;
            const double span     = std::max(0.0, endSeconds - startSeconds);
            const double duration = span / static_cast<double>(count);
            for (int i = 0; i < count; ++i)
            {
                Event e;
                e.startSeconds         = startSeconds + duration * static_cast<double>(i);
                e.durationSeconds      = duration * 0.9;   // small gap for envelope tails
                e.pitchOffsetSemitones = pitchOffsetSemitones;
                e.velocity             = velocity;
                processor.getPattern().addEvent(lane, e);
            }
        }

        // ---- Preset configurators ---------------------------------

        void configureFmBell(B33pProcessor& processor)
        {
            processor.resetToDefaults();
            // Bell-like FM: 2.5 ratio gives an inharmonic spectrum
            // and depth = 6 puts plenty of sidebands above the
            // carrier so the timbre actually "rings" rather than
            // sounding like vibrato.
            const int lane = 0;
            setChoice(processor, ParameterIDs::oscWaveform(lane),
                      static_cast<int>(Oscillator::Waveform::FM));
            setParam (processor, ParameterIDs::basePitchHz(lane),  440.0f);
            setParam (processor, ParameterIDs::fmRatio(lane),     2.5f);
            setParam (processor, ParameterIDs::fmDepth(lane),     6.0f);
            // Slow attack and long release sells the bell-strike
            // envelope shape.
            setParam (processor, ParameterIDs::ampAttack(lane),   0.005f);
            setParam (processor, ParameterIDs::ampDecay(lane),    0.4f);
            setParam (processor, ParameterIDs::ampSustain(lane),  0.0f);
            setParam (processor, ParameterIDs::ampRelease(lane),  1.5f);
            // Wide-open filter; the FM spectrum is what we want
            // to hear.
            setParam (processor, ParameterIDs::filterCutoffHz(lane), 12000.0f);

            processor.getPattern().setLengthSeconds(2.0);
            addEvenlySpacedEvents(processor, lane, 1, 0.1, 2.0, 1.0f, 0.0f);
        }

        void configureResonantStab(B33pProcessor& processor)
        {
            processor.resetToDefaults();
            const int lane = 0;
            setChoice(processor, ParameterIDs::oscWaveform(lane),
                      static_cast<int>(Oscillator::Waveform::Saw));
            setParam (processor, ParameterIDs::basePitchHz(lane),  220.0f);
            setParam (processor, ParameterIDs::ampAttack(lane),   0.0f);
            setParam (processor, ParameterIDs::ampDecay(lane),    0.15f);
            setParam (processor, ParameterIDs::ampSustain(lane),  0.0f);
            setParam (processor, ParameterIDs::ampRelease(lane),  0.05f);

            // Resonant LP with LFO 1 sweeping the cutoff for a
            // squelchy "wow" stab.
            setChoice(processor, ParameterIDs::filterType(lane),
                      static_cast<int>(Filter::Type::Lowpass));
            setParam (processor, ParameterIDs::filterCutoffHz(lane),  800.0f);
            setParam (processor, ParameterIDs::filterResonanceQ(lane), 12.0f);

            // LFO 1 → filter cutoff, slow but deep.
            setChoice(processor, ParameterIDs::lfoShape(lane, 0),
                      static_cast<int>(0));   // Sine
            setParam (processor, ParameterIDs::lfoRateHz(lane, 0), 0.8f);

            setChoice(processor, ParameterIDs::modSlotSource(lane, 0), 1); // LFO 1
            setChoice(processor, ParameterIDs::modSlotDest  (lane, 0),
                      static_cast<int>(ModDestination::FilterCutoff));
            setParam (processor, ParameterIDs::modSlotAmount(lane, 0), 0.7f);

            processor.getPattern().setLengthSeconds(4.0);
            addEvenlySpacedEvents(processor, lane, 8, 0.0, 4.0, 0.85f, 0.0f);
        }

        void configureDelayPad(B33pProcessor& processor)
        {
            processor.resetToDefaults();
            const int lane = 0;
            setChoice(processor, ParameterIDs::oscWaveform(lane),
                      static_cast<int>(Oscillator::Waveform::Triangle));
            setParam (processor, ParameterIDs::basePitchHz(lane),  330.0f);

            // Slow attack + long release for an evolving texture.
            setParam (processor, ParameterIDs::ampAttack(lane),   0.4f);
            setParam (processor, ParameterIDs::ampDecay(lane),    0.5f);
            setParam (processor, ParameterIDs::ampSustain(lane),  0.7f);
            setParam (processor, ParameterIDs::ampRelease(lane),  2.5f);

            // Soft LP rolls off the harshness of the triangle's
            // upper harmonics.
            setParam (processor, ParameterIDs::filterCutoffHz(lane), 2500.0f);

            // Mod FX = Delay with strong feedback for an echoing
            // pad that builds over the pattern.
            setChoice(processor, ParameterIDs::modEffectType(lane),
                      static_cast<int>(ModulationEffect::Type::Delay));
            setParam (processor, ParameterIDs::modEffectParam1(lane), 0.5f);   // ~45 ms
            setParam (processor, ParameterIDs::modEffectParam2(lane), 0.85f);  // high feedback
            setParam (processor, ParameterIDs::modEffectMix(lane),    0.55f);

            processor.getPattern().setLengthSeconds(6.0);
            // Sparse triggers — the delay does most of the rhythmic
            // work.
            Event a; a.startSeconds = 0.0; a.durationSeconds = 0.6; a.pitchOffsetSemitones = 0.0f;
            Event b = a; b.startSeconds = 1.5; b.pitchOffsetSemitones = 5.0f;
            Event c = a; c.startSeconds = 3.0; c.pitchOffsetSemitones = 3.0f;
            Event d = a; d.startSeconds = 4.5; d.pitchOffsetSemitones = -2.0f;
            processor.getPattern().addEvent(lane, a);
            processor.getPattern().addEvent(lane, b);
            processor.getPattern().addEvent(lane, c);
            processor.getPattern().addEvent(lane, d);
        }

        void configureRingModRobot(B33pProcessor& processor)
        {
            processor.resetToDefaults();
            const int lane = 0;
            setChoice(processor, ParameterIDs::oscWaveform(lane),
                      static_cast<int>(Oscillator::Waveform::Ring));
            setParam (processor, ParameterIDs::basePitchHz(lane),  220.0f);
            setParam (processor, ParameterIDs::ringRatio(lane),    3.7f);
            setParam (processor, ParameterIDs::ringMix(lane),      1.0f);
            // Tight envelope for staccato robot speech.
            setParam (processor, ParameterIDs::ampAttack(lane),    0.005f);
            setParam (processor, ParameterIDs::ampDecay(lane),     0.08f);
            setParam (processor, ParameterIDs::ampSustain(lane),   0.0f);
            setParam (processor, ParameterIDs::ampRelease(lane),   0.05f);

            processor.getPattern().setLengthSeconds(2.0);
            // 16 quick chatter hits with random-ish pitch via
            // overrides for variety.
            const float pitches[] = { 0, 4, 7, 2, 5, -3, 0, 9, 4, 0, -5, 7, 2, 0, 12, -2 };
            for (int i = 0; i < 16; ++i)
            {
                Event e;
                e.startSeconds         = 0.05 + 0.12 * static_cast<double>(i);
                e.durationSeconds      = 0.08;
                e.pitchOffsetSemitones = pitches[i];
                e.velocity             = 0.85f;
                e.humanizeAmount       = 0.25f;
                processor.getPattern().addEvent(lane, e);
            }
        }
    }

    std::vector<FactoryPreset> getFactoryPresets()
    {
        return {
            { "Factory - FM Bell",         configureFmBell        },
            { "Factory - Resonant Stab",   configureResonantStab  },
            { "Factory - Delay Pad",       configureDelayPad      },
            { "Factory - Ring Mod Robot",  configureRingModRobot  },
        };
    }
}

#include "ParameterLayout.h"

#include "Core/ParameterIDs.h"
#include "DSP/ModulationMatrix.h"
#include "Pattern/Pattern.h"

namespace B33p
{
    namespace
    {
        constexpr int kParameterVersionHint = 1;

        juce::NormalisableRange<float> skewedRange(float min, float max, float centre)
        {
            juce::NormalisableRange<float> range { min, max };
            range.setSkewForCentre(centre);
            return range;
        }

        std::unique_ptr<juce::AudioParameterFloat> makeFloat(const juce::String& id,
                                                             const juce::String& name,
                                                             juce::NormalisableRange<float> range,
                                                             float defaultValue,
                                                             const juce::String& unitSuffix = {})
        {
            return std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { id, kParameterVersionHint },
                name,
                range,
                defaultValue,
                unitSuffix);
        }

        // One full per-lane parameter set: oscillator choice + eleven
        // continuous params. Display names are lane-prefixed so the
        // host (and any future "show all parameters" UI) shows
        // "Lane 1 Pitch" / "Lane 2 Pitch" / ... clearly.
        void addLaneParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                               int lane)
        {
            const juce::String prefix = "Lane " + juce::String(lane + 1) + " ";

            layout.add(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID { ParameterIDs::oscWaveform(lane), kParameterVersionHint },
                prefix + "Waveform",
                juce::StringArray { "Sine", "Square", "Triangle", "Saw", "Noise", "Custom", "Wavetable", "FM", "Ring" },
                0));

            // Tightened to "musical beep" range so randomisation
            // can't produce inaudible sub-20 Hz or above-4 kHz
            // squeals. 80 Hz = E2-ish, 4 kHz = above the highest
            // beep most projects need.
            layout.add(makeFloat(ParameterIDs::basePitchHz(lane),
                                 prefix + "Base Pitch",
                                 skewedRange(80.0f, 4000.0f, 440.0f),
                                 440.0f, "Hz"));

            // 0..1 morph across the four wavetable slots. Only
            // active when waveform is Wavetable; ignored otherwise.
            layout.add(makeFloat(ParameterIDs::wavetableMorph(lane),
                                 prefix + "Wavetable Morph",
                                 juce::NormalisableRange<float> { 0.0f, 1.0f },
                                 0.0f));

            // FM operator parameters. Both only active when waveform
            // is FM; ignored otherwise. Ratio range covers sub-octave
            // to four octaves up; depth caps at 10 (well into
            // bell/percussion territory). Default depth = 0 so a
            // fresh FM lane plays a clean carrier sine until the
            // user dials depth up.
            layout.add(makeFloat(ParameterIDs::fmRatio(lane),
                                 prefix + "FM Ratio",
                                 skewedRange(0.1f, 16.0f, 1.0f),
                                 1.0f));

            layout.add(makeFloat(ParameterIDs::fmDepth(lane),
                                 prefix + "FM Depth",
                                 skewedRange(0.0f, 10.0f, 1.0f),
                                 0.0f));

            // Ring modulator parameters. Default ratio of 2.0 picks
            // a non-unity starting point (a unison ring mod just
            // shifts the sine into a 2x sine, which is hard to
            // distinguish from a regular sine at first glance).
            // Default mix = 1.0 so flipping into Ring mode produces
            // the full ring-modulated sound immediately.
            layout.add(makeFloat(ParameterIDs::ringRatio(lane),
                                 prefix + "Ring Ratio",
                                 skewedRange(0.1f, 16.0f, 2.0f),
                                 2.0f));

            layout.add(makeFloat(ParameterIDs::ringMix(lane),
                                 prefix + "Ring Mix",
                                 juce::NormalisableRange<float> { 0.0f, 1.0f },
                                 1.0f));

            layout.add(makeFloat(ParameterIDs::ampAttack(lane),
                                 prefix + "Amp Attack",
                                 skewedRange(0.0f, 5.0f, 0.1f),
                                 0.005f, "s"));

            layout.add(makeFloat(ParameterIDs::ampDecay(lane),
                                 prefix + "Amp Decay",
                                 skewedRange(0.0f, 5.0f, 0.1f),
                                 0.05f, "s"));

            layout.add(makeFloat(ParameterIDs::ampSustain(lane),
                                 prefix + "Amp Sustain",
                                 juce::NormalisableRange<float> { 0.0f, 1.0f },
                                 1.0f));

            layout.add(makeFloat(ParameterIDs::ampRelease(lane),
                                 prefix + "Amp Release",
                                 skewedRange(0.0f, 5.0f, 0.1f),
                                 0.1f, "s"));

            // Filter type chooses among the five filter modes the
            // Filter class implements. The cutoff / resonance /
            // vowel parameters carry different meanings per type;
            // see Filter.h for the mapping.
            layout.add(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID { ParameterIDs::filterType(lane), kParameterVersionHint },
                prefix + "Filter Type",
                juce::StringArray { "Lowpass", "Highpass", "Bandpass", "Comb", "Formant" },
                0));

            // 200 Hz lower bound: a randomised cutoff can't drop
            // below the fundamental of the lowest pitch and silence
            // the voice.
            layout.add(makeFloat(ParameterIDs::filterCutoffHz(lane),
                                 prefix + "Filter Cutoff",
                                 skewedRange(200.0f, 20000.0f, 1000.0f),
                                 20000.0f, "Hz"));

            layout.add(makeFloat(ParameterIDs::filterResonanceQ(lane),
                                 prefix + "Filter Resonance",
                                 juce::NormalisableRange<float> { 0.1f, 20.0f },
                                 0.707f));

            // Vowel position used by Formant mode only. 0 = A,
            // 1 = U with E / I / O at 0.25 / 0.5 / 0.75. Adjacent
            // vowels' formant frequencies are linearly interpolated.
            layout.add(makeFloat(ParameterIDs::filterVowel(lane),
                                 prefix + "Filter Vowel",
                                 juce::NormalisableRange<float> { 0.0f, 1.0f },
                                 0.0f));

            layout.add(makeFloat(ParameterIDs::bitcrushBitDepth(lane),
                                 prefix + "Bit Depth",
                                 juce::NormalisableRange<float> { 1.0f, 16.0f },
                                 16.0f, "bits"));

            // 1 kHz lower bound: a randomised sample rate can't
            // drop below ~Nyquist of the lowest pitch and silence
            // / completely-alias the voice.
            layout.add(makeFloat(ParameterIDs::bitcrushSampleRateHz(lane),
                                 prefix + "Crush Sample Rate",
                                 skewedRange(1000.0f, 48000.0f, 8000.0f),
                                 48000.0f, "Hz"));

            layout.add(makeFloat(ParameterIDs::distortionDrive(lane),
                                 prefix + "Distortion Drive",
                                 juce::NormalisableRange<float> { 0.1f, 100.0f },
                                 1.0f));

            // Modulation effect slot at the end of the chain. Type
            // selects among None / Chorus / Reverb / Delay / Flanger
            // / Phaser; the three continuous parameters carry
            // type-dependent semantics (see ModulationEffect.h).
            layout.add(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID { ParameterIDs::modEffectType(lane), kParameterVersionHint },
                prefix + "Mod Effect Type",
                juce::StringArray { "None", "Chorus", "Reverb", "Delay", "Flanger", "Phaser" },
                0));

            layout.add(makeFloat(ParameterIDs::modEffectParam1(lane),
                                 prefix + "Mod Effect P1",
                                 juce::NormalisableRange<float> { 0.0f, 1.0f },
                                 0.5f));

            layout.add(makeFloat(ParameterIDs::modEffectParam2(lane),
                                 prefix + "Mod Effect P2",
                                 juce::NormalisableRange<float> { 0.0f, 1.0f },
                                 0.5f));

            layout.add(makeFloat(ParameterIDs::modEffectMix(lane),
                                 prefix + "Mod Effect Mix",
                                 juce::NormalisableRange<float> { 0.0f, 1.0f },
                                 0.5f));

            layout.add(makeFloat(ParameterIDs::voiceGain(lane),
                                 prefix + "Voice Gain",
                                 juce::NormalisableRange<float> { 0.0f, 10.0f },
                                 1.0f));

            // LFOs — two per lane. Default rate of 1 Hz puts them
            // squarely in "audible motion" territory; default shape
            // is Sine because it's the smoothest modulator and the
            // least surprising starting point.
            for (int i = 0; i < kNumLfosPerLane; ++i)
            {
                const juce::String lfoPrefix = prefix + "LFO " + juce::String(i + 1) + " ";

                layout.add(std::make_unique<juce::AudioParameterChoice>(
                    juce::ParameterID { ParameterIDs::lfoShape(lane, i), kParameterVersionHint },
                    lfoPrefix + "Shape",
                    juce::StringArray { "Sine", "Triangle", "Saw", "Square" },
                    0));

                layout.add(makeFloat(ParameterIDs::lfoRateHz(lane, i),
                                     lfoPrefix + "Rate",
                                     skewedRange(0.0f, 30.0f, 1.0f),
                                     1.0f, "Hz"));
            }

            // Modulation matrix slots. Source / dest both default
            // to None so a fresh project's matrix is inert until
            // the user wires something. Amount centres on 0 with
            // a [-1, +1] range — a slot with non-zero amount but
            // None source contributes nothing.
            for (int i = 0; i < kNumModSlots; ++i)
            {
                const juce::String slotPrefix = prefix + "Mod " + juce::String(i + 1) + " ";

                layout.add(std::make_unique<juce::AudioParameterChoice>(
                    juce::ParameterID { ParameterIDs::modSlotSource(lane, i), kParameterVersionHint },
                    slotPrefix + "Source",
                    juce::StringArray { "None", "LFO 1", "LFO 2" },
                    0));

                layout.add(std::make_unique<juce::AudioParameterChoice>(
                    juce::ParameterID { ParameterIDs::modSlotDest(lane, i), kParameterVersionHint },
                    slotPrefix + "Destination",
                    juce::StringArray {
                        "None",
                        "Osc Pitch",
                        "Wavetable Morph",
                        "FM Depth",
                        "Ring Mix",
                        "Filter Cutoff",
                        "Filter Resonance",
                        "Distortion Drive",
                        "Mod FX P1",
                        "Mod FX P2",
                        "Mod FX Mix",
                        "Voice Gain"
                    },
                    0));

                layout.add(makeFloat(ParameterIDs::modSlotAmount(lane, i),
                                     slotPrefix + "Amount",
                                     juce::NormalisableRange<float> { -1.0f, 1.0f },
                                     0.0f));
            }
        }
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
            addLaneParameters(layout, lane);
        return layout;
    }
}

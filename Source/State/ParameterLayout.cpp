#include "ParameterLayout.h"

#include "Core/ParameterIDs.h"
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
                juce::StringArray { "Sine", "Square", "Triangle", "Saw", "Noise" },
                0));

            // Tightened to "musical beep" range so randomisation
            // can't produce inaudible sub-20 Hz or above-4 kHz
            // squeals. 80 Hz = E2-ish, 4 kHz = above the highest
            // beep most projects need.
            layout.add(makeFloat(ParameterIDs::basePitchHz(lane),
                                 prefix + "Base Pitch",
                                 skewedRange(80.0f, 4000.0f, 440.0f),
                                 440.0f, "Hz"));

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

            layout.add(makeFloat(ParameterIDs::voiceGain(lane),
                                 prefix + "Voice Gain",
                                 juce::NormalisableRange<float> { 0.0f, 10.0f },
                                 1.0f));
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

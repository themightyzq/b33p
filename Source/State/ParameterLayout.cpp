#include "ParameterLayout.h"

#include "Core/ParameterIDs.h"

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

        std::unique_ptr<juce::AudioParameterFloat> makeFloat(const char* id,
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
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        // Oscillator — choice order MUST match Oscillator::Waveform
        // so an int index maps directly onto the enum at wiring time.
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { ParameterIDs::oscWaveform, kParameterVersionHint },
            "Waveform",
            juce::StringArray { "Sine", "Square", "Triangle", "Saw", "Noise" },
            0));

        layout.add(makeFloat(ParameterIDs::basePitchHz,
                             "Base Pitch",
                             skewedRange(20.0f, 20000.0f, 440.0f),
                             440.0f,
                             "Hz"));

        layout.add(makeFloat(ParameterIDs::ampAttack,
                             "Amp Attack",
                             skewedRange(0.0f, 5.0f, 0.1f),
                             0.005f,
                             "s"));

        layout.add(makeFloat(ParameterIDs::ampDecay,
                             "Amp Decay",
                             skewedRange(0.0f, 5.0f, 0.1f),
                             0.05f,
                             "s"));

        layout.add(makeFloat(ParameterIDs::ampSustain,
                             "Amp Sustain",
                             juce::NormalisableRange<float> { 0.0f, 1.0f },
                             1.0f));

        layout.add(makeFloat(ParameterIDs::ampRelease,
                             "Amp Release",
                             skewedRange(0.0f, 5.0f, 0.1f),
                             0.1f,
                             "s"));

        layout.add(makeFloat(ParameterIDs::filterCutoffHz,
                             "Filter Cutoff",
                             skewedRange(20.0f, 20000.0f, 1000.0f),
                             20000.0f,
                             "Hz"));

        layout.add(makeFloat(ParameterIDs::filterResonanceQ,
                             "Filter Resonance",
                             juce::NormalisableRange<float> { 0.1f, 20.0f },
                             0.707f));

        layout.add(makeFloat(ParameterIDs::bitcrushBitDepth,
                             "Bit Depth",
                             juce::NormalisableRange<float> { 1.0f, 16.0f },
                             16.0f,
                             "bits"));

        layout.add(makeFloat(ParameterIDs::bitcrushSampleRateHz,
                             "Crush Sample Rate",
                             skewedRange(200.0f, 48000.0f, 8000.0f),
                             48000.0f,
                             "Hz"));

        layout.add(makeFloat(ParameterIDs::distortionDrive,
                             "Distortion Drive",
                             juce::NormalisableRange<float> { 0.1f, 100.0f },
                             1.0f));

        layout.add(makeFloat(ParameterIDs::voiceGain,
                             "Voice Gain",
                             juce::NormalisableRange<float> { 0.0f, 10.0f },
                             1.0f));

        return layout;
    }
}

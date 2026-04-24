#include "OscillatorSection.h"

#include "Core/ParameterIDs.h"

namespace B33p
{
    OscillatorSection::OscillatorSection(juce::AudioProcessorValueTreeState& apvts)
        : Section("Oscillator"),
          basePitchAttachment(apvts, ParameterIDs::basePitchHz, basePitchSlider.getSlider())
    {
        // Item IDs (1..5) match B33p::Oscillator::Waveform enum order
        // so the APVTS choice index maps cleanly at wiring time.
        waveformSelector.addItem("Sine",     1);
        waveformSelector.addItem("Square",   2);
        waveformSelector.addItem("Triangle", 3);
        waveformSelector.addItem("Saw",      4);
        waveformSelector.addItem("Noise",    5);

        waveformAttachment = std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, ParameterIDs::oscWaveform, waveformSelector);

        addAndMakeVisible(waveformSelector);
        addAndMakeVisible(basePitchSlider);
    }

    void OscillatorSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kComboHeight = 26;
        constexpr int kGap         = 8;

        waveformSelector.setBounds(bounds.removeFromTop(kComboHeight));
        bounds.removeFromTop(kGap);
        basePitchSlider.setBounds(bounds);
    }
}

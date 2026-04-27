#include "OscillatorSection.h"

#include "Core/ParameterIDs.h"
#include "RandomizerWiring.h"
#include "SliderFormatting.h"

namespace B33p
{
    OscillatorSection::OscillatorSection(B33pProcessor& processor)
        : Section("Oscillator"),
          basePitchAttachment(processor.getApvts(),
                              ParameterIDs::basePitchHz(0),
                              basePitchSlider.getSlider())
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
                processor.getApvts(), ParameterIDs::oscWaveform(0), waveformSelector);

        addAndMakeVisible(waveformSelector);
        addAndMakeVisible(waveformDice);
        addAndMakeVisible(waveformLock);
        addAndMakeVisible(basePitchSlider);

        wireRandomizerButtons(processor, waveformDice, waveformLock,
                              ParameterIDs::oscWaveform(0));
        basePitchSlider.attachRandomizer(processor, ParameterIDs::basePitchHz(0));

        SliderFormatting::applyHz(basePitchSlider.getSlider());
        SliderFormatting::applyDoubleClickReset(basePitchSlider.getSlider(),
                                                processor.getApvts(),
                                                ParameterIDs::basePitchHz(0));

        waveformSelector.setTooltip("Oscillator waveform");
        basePitchSlider .setTooltip("Base pitch of the oscillator");
    }

    void OscillatorSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kComboHeight  = 26;
        constexpr int kButtonWidth  = 32;
        constexpr int kButtonGap    = 2;
        constexpr int kComboGap     = 4;
        constexpr int kRowGap       = 8;

        auto topRow = bounds.removeFromTop(kComboHeight);
        waveformLock.setBounds(topRow.removeFromRight(kButtonWidth));
        topRow.removeFromRight(kButtonGap);
        waveformDice.setBounds(topRow.removeFromRight(kButtonWidth));
        topRow.removeFromRight(kComboGap);
        waveformSelector.setBounds(topRow);

        bounds.removeFromTop(kRowGap);
        basePitchSlider.setBounds(bounds);
    }
}

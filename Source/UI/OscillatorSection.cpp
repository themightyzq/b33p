#include "OscillatorSection.h"

#include "Core/ParameterIDs.h"
#include "RandomizerWiring.h"
#include "SliderFormatting.h"

namespace B33p
{
    OscillatorSection::OscillatorSection(B33pProcessor& processorRef)
        : Section("Oscillator"),
          processor(processorRef)
    {
        // Item IDs (1..6) match B33p::Oscillator::Waveform enum order
        // so the APVTS choice index maps cleanly at wiring time.
        waveformSelector.addItem("Sine",     1);
        waveformSelector.addItem("Square",   2);
        waveformSelector.addItem("Triangle", 3);
        waveformSelector.addItem("Saw",      4);
        waveformSelector.addItem("Noise",    5);
        waveformSelector.addItem("Custom",   6);

        addAndMakeVisible(waveformSelector);
        addAndMakeVisible(waveformDice);
        addAndMakeVisible(waveformLock);
        addAndMakeVisible(basePitchSlider);

        waveformSelector.setTooltip("Oscillator waveform");
        basePitchSlider .setTooltip("Base pitch of the oscillator");

        retargetLane(processor.getSelectedLane());
    }

    void OscillatorSection::retargetLane(int lane)
    {
        // Drop the old attachments first; constructing a new one
        // immediately starts pushing parameter values into the
        // controls, which is what we want.
        waveformAttachment.reset();
        basePitchAttachment.reset();

        waveformAttachment = std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getApvts(),
                ParameterIDs::oscWaveform(lane),
                waveformSelector);

        basePitchAttachment = std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.getApvts(),
                ParameterIDs::basePitchHz(lane),
                basePitchSlider.getSlider());

        wireRandomizerButtons(processor, waveformDice, waveformLock,
                              ParameterIDs::oscWaveform(lane));
        basePitchSlider.attachRandomizer(processor, ParameterIDs::basePitchHz(lane));

        // SliderAttachment installs its own textFromValueFunction at
        // construction, so re-apply our formatting AFTER the
        // attachment exists.
        SliderFormatting::applyHz(basePitchSlider.getSlider());
        SliderFormatting::applyDoubleClickReset(basePitchSlider.getSlider(),
                                                processor.getApvts(),
                                                ParameterIDs::basePitchHz(lane));

        setTitleSuffix(processor.laneTitleSuffix(lane));
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

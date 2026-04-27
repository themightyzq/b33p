#include "MasterSection.h"

#include "Core/ParameterIDs.h"
#include "SliderFormatting.h"

namespace B33p
{
    MasterSection::MasterSection(B33pProcessor& processorRef)
        : Section("Master"),
          processor(processorRef)
    {
        addAndMakeVisible(gainSlider);

        auditionButton.onClick = [this]
        {
            processor.triggerAudition();
            flashAuditionButton();
        };
        addAndMakeVisible(auditionButton);

        diceAllButton.onClick = [this]
        {
            juce::Random rng;
            processor.getRandomizer().rollAllUnlocked(rng);
        };
        addAndMakeVisible(diceAllButton);

        gainSlider    .setTooltip("Master output level");
        auditionButton.setTooltip("Play a single beep with the current settings (Shift+Space)");
        diceAllButton .setTooltip("Roll random values for every unlocked parameter");

        retargetLane(processor.getSelectedLane());
    }

    void MasterSection::retargetLane(int lane)
    {
        gainAttachment.reset();
        gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::voiceGain(lane), gainSlider.getSlider());

        gainSlider.attachRandomizer(processor, ParameterIDs::voiceGain(lane));
        SliderFormatting::applyDecimal(gainSlider.getSlider(), 2);
        SliderFormatting::applyDoubleClickReset(gainSlider.getSlider(),
                                                processor.getApvts(),
                                                ParameterIDs::voiceGain(lane));

        setTitleSuffix(processor.laneTitleSuffix(lane));
    }

    void MasterSection::flashAuditionButton()
    {
        // Brief amber tint so the user gets a visual confirmation
        // that the click registered — the audition is a fire-and-
        // forget half-second beep with no other on-screen feedback.
        auditionButton.setColour(juce::TextButton::buttonColourId,
                                  juce::Colour::fromRGB(220, 140, 60));
        startTimer(180);
    }

    void MasterSection::timerCallback()
    {
        auditionButton.removeColour(juce::TextButton::buttonColourId);
        stopTimer();
    }

    void MasterSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kSliderWidth  = 140;
        constexpr int kButtonHeight = 28;
        constexpr int kButtonGap    = 6;
        constexpr int kRowGap       = 6;

        auto buttonRow = bounds.removeFromBottom(kButtonHeight);
        bounds.removeFromBottom(kRowGap);

        const int xSlider = bounds.getX() + (bounds.getWidth() - kSliderWidth) / 2;
        gainSlider.setBounds(xSlider, bounds.getY(), kSliderWidth, bounds.getHeight());

        const int cellWidth = (buttonRow.getWidth() - kButtonGap) / 2;
        diceAllButton.setBounds(buttonRow.removeFromLeft(cellWidth));
        buttonRow.removeFromLeft(kButtonGap);
        auditionButton.setBounds(buttonRow);
    }
}

#include "MasterSection.h"

#include "Core/ParameterIDs.h"
#include "SliderFormatting.h"

namespace B33p
{
    MasterSection::MasterSection(B33pProcessor& processor)
        : Section("Master"),
          gainAttachment(processor.getApvts(), ParameterIDs::voiceGain, gainSlider.getSlider())
    {
        addAndMakeVisible(gainSlider);

        auditionButton.onClick = [&processor] { processor.triggerAudition(); };
        addAndMakeVisible(auditionButton);

        diceAllButton.onClick = [&processor]
        {
            juce::Random rng;
            processor.getRandomizer().rollAllUnlocked(rng);
        };
        addAndMakeVisible(diceAllButton);

        gainSlider.attachRandomizer(processor, ParameterIDs::voiceGain);

        SliderFormatting::applyDecimal(gainSlider.getSlider(), 2);

        gainSlider    .setTooltip("Master output level");
        auditionButton.setTooltip("Play a single beep with the current settings (Space)");
        diceAllButton .setTooltip("Roll random values for every unlocked parameter");
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

#include "MasterSection.h"

#include "Core/ParameterIDs.h"

namespace B33p
{
    MasterSection::MasterSection(B33pProcessor& processor)
        : Section("Master"),
          gainAttachment(processor.getApvts(), ParameterIDs::voiceGain, gainSlider.getSlider())
    {
        addAndMakeVisible(gainSlider);

        auditionButton.onClick = [&processor] { processor.triggerAudition(); };
        addAndMakeVisible(auditionButton);

        gainSlider.attachRandomizer(processor, ParameterIDs::voiceGain);
    }

    void MasterSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kSliderWidth   = 140;
        constexpr int kButtonHeight  = 28;
        constexpr int kButtonWidth   = 120;
        constexpr int kGap           = 6;

        auto buttonRow = bounds.removeFromBottom(kButtonHeight);
        bounds.removeFromBottom(kGap);

        const int xSlider = bounds.getX() + (bounds.getWidth() - kSliderWidth) / 2;
        gainSlider.setBounds(xSlider, bounds.getY(), kSliderWidth, bounds.getHeight());

        const int xButton = buttonRow.getX() + (buttonRow.getWidth() - kButtonWidth) / 2;
        auditionButton.setBounds(xButton, buttonRow.getY(), kButtonWidth, kButtonHeight);
    }
}

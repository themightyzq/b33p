#include "MasterSection.h"

#include "Core/ParameterIDs.h"

namespace B33p
{
    MasterSection::MasterSection(juce::AudioProcessorValueTreeState& apvts)
        : Section("Master"),
          gainAttachment(apvts, ParameterIDs::voiceGain, gainSlider.getSlider())
    {
        addAndMakeVisible(gainSlider);
    }

    void MasterSection::resized()
    {
        auto bounds = getContentBounds();

        // One slider centred horizontally inside the section — the
        // audition button will sit next to it in a later task.
        constexpr int kSliderWidth = 140;
        const int x = bounds.getX() + (bounds.getWidth() - kSliderWidth) / 2;
        gainSlider.setBounds(x, bounds.getY(), kSliderWidth, bounds.getHeight());
    }
}

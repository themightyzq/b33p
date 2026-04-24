#include "EffectsSection.h"

#include "Core/ParameterIDs.h"

namespace B33p
{
    EffectsSection::EffectsSection(juce::AudioProcessorValueTreeState& apvts)
        : Section("Effects"),
          bitDepthAttachment (apvts, ParameterIDs::bitcrushBitDepth,     bitDepthSlider.getSlider()),
          crushRateAttachment(apvts, ParameterIDs::bitcrushSampleRateHz, crushRateSlider.getSlider()),
          driveAttachment    (apvts, ParameterIDs::distortionDrive,      driveSlider.getSlider())
    {
        addAndMakeVisible(bitDepthSlider);
        addAndMakeVisible(crushRateSlider);
        addAndMakeVisible(driveSlider);
    }

    void EffectsSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kGap = 8;
        const int cellWidth = (bounds.getWidth() - 2 * kGap) / 3;

        bitDepthSlider.setBounds(bounds.removeFromLeft(cellWidth));
        bounds.removeFromLeft(kGap);
        crushRateSlider.setBounds(bounds.removeFromLeft(cellWidth));
        bounds.removeFromLeft(kGap);
        driveSlider.setBounds(bounds);
    }
}

#include "EffectsSection.h"

#include "Core/ParameterIDs.h"

namespace B33p
{
    EffectsSection::EffectsSection(B33pProcessor& processor)
        : Section("Effects"),
          bitDepthAttachment (processor.getApvts(), ParameterIDs::bitcrushBitDepth,     bitDepthSlider.getSlider()),
          crushRateAttachment(processor.getApvts(), ParameterIDs::bitcrushSampleRateHz, crushRateSlider.getSlider()),
          driveAttachment    (processor.getApvts(), ParameterIDs::distortionDrive,      driveSlider.getSlider())
    {
        addAndMakeVisible(bitDepthSlider);
        addAndMakeVisible(crushRateSlider);
        addAndMakeVisible(driveSlider);

        bitDepthSlider .attachRandomizer(processor, ParameterIDs::bitcrushBitDepth);
        crushRateSlider.attachRandomizer(processor, ParameterIDs::bitcrushSampleRateHz);
        driveSlider    .attachRandomizer(processor, ParameterIDs::distortionDrive);
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

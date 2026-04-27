#include "EffectsSection.h"

#include "Core/ParameterIDs.h"
#include "SliderFormatting.h"

namespace B33p
{
    EffectsSection::EffectsSection(B33pProcessor& processor)
        : Section("Effects"),
          bitDepthAttachment (processor.getApvts(), ParameterIDs::bitcrushBitDepth(0),     bitDepthSlider.getSlider()),
          crushRateAttachment(processor.getApvts(), ParameterIDs::bitcrushSampleRateHz(0), crushRateSlider.getSlider()),
          driveAttachment    (processor.getApvts(), ParameterIDs::distortionDrive(0),      driveSlider.getSlider())
    {
        addAndMakeVisible(bitDepthSlider);
        addAndMakeVisible(crushRateSlider);
        addAndMakeVisible(driveSlider);

        bitDepthSlider .attachRandomizer(processor, ParameterIDs::bitcrushBitDepth(0));
        crushRateSlider.attachRandomizer(processor, ParameterIDs::bitcrushSampleRateHz(0));
        driveSlider    .attachRandomizer(processor, ParameterIDs::distortionDrive(0));

        SliderFormatting::applyInteger(bitDepthSlider .getSlider(), " bits");
        SliderFormatting::applyHz     (crushRateSlider.getSlider());
        SliderFormatting::applyDecimal(driveSlider    .getSlider(), 2);

        SliderFormatting::applyDoubleClickReset(bitDepthSlider .getSlider(), processor.getApvts(), ParameterIDs::bitcrushBitDepth(0));
        SliderFormatting::applyDoubleClickReset(crushRateSlider.getSlider(), processor.getApvts(), ParameterIDs::bitcrushSampleRateHz(0));
        SliderFormatting::applyDoubleClickReset(driveSlider    .getSlider(), processor.getApvts(), ParameterIDs::distortionDrive(0));

        bitDepthSlider .setTooltip("Bitcrush: lower bit depth = grittier, more 8-bit");
        crushRateSlider.setTooltip("Sample rate reduction: lower = more aliased / lo-fi");
        driveSlider    .setTooltip("Distortion drive: pushes the signal into soft clipping");
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

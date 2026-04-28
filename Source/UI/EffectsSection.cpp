#include "EffectsSection.h"

#include "Core/ParameterIDs.h"
#include "SliderFormatting.h"

namespace B33p
{
    EffectsSection::EffectsSection(B33pProcessor& processorRef)
        : Section("Effects"),
          processor(processorRef)
    {
        addAndMakeVisible(bitDepthSlider);
        addAndMakeVisible(crushRateSlider);
        addAndMakeVisible(driveSlider);

        bitDepthSlider .setTooltip("Bitcrush: lower bit depth = grittier, more 8-bit");
        crushRateSlider.setTooltip("Sample rate reduction: lower = more aliased / lo-fi");
        driveSlider    .setTooltip("Distortion drive: pushes the signal into soft clipping");

        retargetLane(processor.getSelectedLane());
    }

    void EffectsSection::retargetLane(int lane)
    {
        bitDepthAttachment.reset();
        crushRateAttachment.reset();
        driveAttachment.reset();

        bitDepthAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::bitcrushBitDepth(lane),     bitDepthSlider .getSlider());
        crushRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::bitcrushSampleRateHz(lane), crushRateSlider.getSlider());
        driveAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::distortionDrive(lane),      driveSlider    .getSlider());

        bitDepthSlider .attachRandomizer(processor, ParameterIDs::bitcrushBitDepth(lane));
        crushRateSlider.attachRandomizer(processor, ParameterIDs::bitcrushSampleRateHz(lane));
        driveSlider    .attachRandomizer(processor, ParameterIDs::distortionDrive(lane));

        SliderFormatting::applyInteger(bitDepthSlider .getSlider(), " bits");
        SliderFormatting::applyHz     (crushRateSlider.getSlider());
        SliderFormatting::applyDecimal(driveSlider    .getSlider(), 2);

        SliderFormatting::applyDoubleClickReset(bitDepthSlider .getSlider(), processor.getApvts(), ParameterIDs::bitcrushBitDepth(lane));
        SliderFormatting::applyDoubleClickReset(crushRateSlider.getSlider(), processor.getApvts(), ParameterIDs::bitcrushSampleRateHz(lane));
        SliderFormatting::applyDoubleClickReset(driveSlider    .getSlider(), processor.getApvts(), ParameterIDs::distortionDrive(lane));

        setTitleSuffix(processor.laneTitleSuffix(lane));
        setAccentColour(processor.laneAccentColour(lane));
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

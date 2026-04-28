#include "FilterSection.h"

#include "Core/ParameterIDs.h"
#include "SliderFormatting.h"

namespace B33p
{
    FilterSection::FilterSection(B33pProcessor& processorRef)
        : Section("Filter"),
          processor(processorRef)
    {
        addAndMakeVisible(cutoffSlider);
        addAndMakeVisible(resonanceSlider);

        cutoffSlider   .setTooltip("Lowpass cutoff: frequencies above are rolled off");
        resonanceSlider.setTooltip("Emphasis around the cutoff: higher = more whistly");

        retargetLane(processor.getSelectedLane());
    }

    void FilterSection::retargetLane(int lane)
    {
        cutoffAttachment.reset();
        resonanceAttachment.reset();

        cutoffAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::filterCutoffHz(lane),   cutoffSlider   .getSlider());
        resonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::filterResonanceQ(lane), resonanceSlider.getSlider());

        cutoffSlider   .attachRandomizer(processor, ParameterIDs::filterCutoffHz(lane));
        resonanceSlider.attachRandomizer(processor, ParameterIDs::filterResonanceQ(lane));

        SliderFormatting::applyHz     (cutoffSlider   .getSlider());
        SliderFormatting::applyDecimal(resonanceSlider.getSlider(), 2);

        SliderFormatting::applyDoubleClickReset(cutoffSlider   .getSlider(), processor.getApvts(), ParameterIDs::filterCutoffHz(lane));
        SliderFormatting::applyDoubleClickReset(resonanceSlider.getSlider(), processor.getApvts(), ParameterIDs::filterResonanceQ(lane));

        setTitleSuffix(processor.laneTitleSuffix(lane));
        setAccentColour(processor.laneAccentColour(lane));
    }

    void FilterSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kGap = 8;
        const int cellWidth = (bounds.getWidth() - kGap) / 2;

        cutoffSlider.setBounds(bounds.removeFromLeft(cellWidth));
        bounds.removeFromLeft(kGap);
        resonanceSlider.setBounds(bounds);
    }
}

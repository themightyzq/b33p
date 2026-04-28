#include "AmpEnvSection.h"

#include "Core/ParameterIDs.h"
#include "SliderFormatting.h"

namespace B33p
{
    AmpEnvSection::AmpEnvSection(B33pProcessor& processorRef)
        : Section("Amp Envelope"),
          processor(processorRef),
          visualizer(processorRef.getApvts())
    {
        addAndMakeVisible(visualizer);
        addAndMakeVisible(attackSlider);
        addAndMakeVisible(decaySlider);
        addAndMakeVisible(sustainSlider);
        addAndMakeVisible(releaseSlider);

        attackSlider .setTooltip("Time to reach full volume after a note starts");
        decaySlider  .setTooltip("Time to fall from peak down to the sustain level");
        sustainSlider.setTooltip("Held volume while the note is on");
        releaseSlider.setTooltip("Time to fade to silence after the note ends");

        retargetLane(processor.getSelectedLane());
    }

    void AmpEnvSection::retargetLane(int lane)
    {
        attackAttachment.reset();
        decayAttachment.reset();
        sustainAttachment.reset();
        releaseAttachment.reset();

        attackAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::ampAttack(lane),  attackSlider .getSlider());
        decayAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::ampDecay(lane),   decaySlider  .getSlider());
        sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::ampSustain(lane), sustainSlider.getSlider());
        releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::ampRelease(lane), releaseSlider.getSlider());

        attackSlider .attachRandomizer(processor, ParameterIDs::ampAttack(lane));
        decaySlider  .attachRandomizer(processor, ParameterIDs::ampDecay(lane));
        sustainSlider.attachRandomizer(processor, ParameterIDs::ampSustain(lane));
        releaseSlider.attachRandomizer(processor, ParameterIDs::ampRelease(lane));

        // SliderAttachment installs its own textFromValueFunction, so
        // re-apply our formatting AFTER the attachment exists.
        SliderFormatting::applySeconds(attackSlider .getSlider());
        SliderFormatting::applySeconds(decaySlider  .getSlider());
        SliderFormatting::applyPercent(sustainSlider.getSlider());
        SliderFormatting::applySeconds(releaseSlider.getSlider());

        SliderFormatting::applyDoubleClickReset(attackSlider .getSlider(), processor.getApvts(), ParameterIDs::ampAttack(lane));
        SliderFormatting::applyDoubleClickReset(decaySlider  .getSlider(), processor.getApvts(), ParameterIDs::ampDecay(lane));
        SliderFormatting::applyDoubleClickReset(sustainSlider.getSlider(), processor.getApvts(), ParameterIDs::ampSustain(lane));
        SliderFormatting::applyDoubleClickReset(releaseSlider.getSlider(), processor.getApvts(), ParameterIDs::ampRelease(lane));

        visualizer.retargetLane(lane);
        setTitleSuffix(processor.laneTitleSuffix(lane));
        setAccentColour(processor.laneAccentColour(lane));
    }

    void AmpEnvSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kVisualizerHeight = 90;
        constexpr int kVisualizerGap    = 6;

        visualizer.setBounds(bounds.removeFromTop(kVisualizerHeight));
        bounds.removeFromTop(kVisualizerGap);

        constexpr int kGap = 4;
        const int cellWidth = (bounds.getWidth() - 3 * kGap) / 4;

        attackSlider.setBounds(bounds.removeFromLeft(cellWidth));
        bounds.removeFromLeft(kGap);
        decaySlider.setBounds(bounds.removeFromLeft(cellWidth));
        bounds.removeFromLeft(kGap);
        sustainSlider.setBounds(bounds.removeFromLeft(cellWidth));
        bounds.removeFromLeft(kGap);
        releaseSlider.setBounds(bounds);
    }
}

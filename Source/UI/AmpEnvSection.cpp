#include "AmpEnvSection.h"

#include "Core/ParameterIDs.h"
#include "SliderFormatting.h"

namespace B33p
{
    AmpEnvSection::AmpEnvSection(B33pProcessor& processor)
        : Section("Amp Envelope"),
          visualizer(processor.getApvts()),
          attackAttachment (processor.getApvts(), ParameterIDs::ampAttack(0),  attackSlider.getSlider()),
          decayAttachment  (processor.getApvts(), ParameterIDs::ampDecay(0),   decaySlider.getSlider()),
          sustainAttachment(processor.getApvts(), ParameterIDs::ampSustain(0), sustainSlider.getSlider()),
          releaseAttachment(processor.getApvts(), ParameterIDs::ampRelease(0), releaseSlider.getSlider())
    {
        addAndMakeVisible(visualizer);
        addAndMakeVisible(attackSlider);
        addAndMakeVisible(decaySlider);
        addAndMakeVisible(sustainSlider);
        addAndMakeVisible(releaseSlider);

        attackSlider .attachRandomizer(processor, ParameterIDs::ampAttack(0));
        decaySlider  .attachRandomizer(processor, ParameterIDs::ampDecay(0));
        sustainSlider.attachRandomizer(processor, ParameterIDs::ampSustain(0));
        releaseSlider.attachRandomizer(processor, ParameterIDs::ampRelease(0));

        SliderFormatting::applySeconds(attackSlider .getSlider());
        SliderFormatting::applySeconds(decaySlider  .getSlider());
        SliderFormatting::applyPercent(sustainSlider.getSlider());
        SliderFormatting::applySeconds(releaseSlider.getSlider());

        SliderFormatting::applyDoubleClickReset(attackSlider .getSlider(), processor.getApvts(), ParameterIDs::ampAttack(0));
        SliderFormatting::applyDoubleClickReset(decaySlider  .getSlider(), processor.getApvts(), ParameterIDs::ampDecay(0));
        SliderFormatting::applyDoubleClickReset(sustainSlider.getSlider(), processor.getApvts(), ParameterIDs::ampSustain(0));
        SliderFormatting::applyDoubleClickReset(releaseSlider.getSlider(), processor.getApvts(), ParameterIDs::ampRelease(0));

        attackSlider .setTooltip("Time to reach full volume after a note starts");
        decaySlider  .setTooltip("Time to fall from peak down to the sustain level");
        sustainSlider.setTooltip("Held volume while the note is on");
        releaseSlider.setTooltip("Time to fade to silence after the note ends");
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

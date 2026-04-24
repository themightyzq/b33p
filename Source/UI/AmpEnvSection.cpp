#include "AmpEnvSection.h"

#include "Core/ParameterIDs.h"

namespace B33p
{
    AmpEnvSection::AmpEnvSection(B33pProcessor& processor)
        : Section("Amp Envelope"),
          visualizer(processor.getApvts()),
          attackAttachment (processor.getApvts(), ParameterIDs::ampAttack,  attackSlider.getSlider()),
          decayAttachment  (processor.getApvts(), ParameterIDs::ampDecay,   decaySlider.getSlider()),
          sustainAttachment(processor.getApvts(), ParameterIDs::ampSustain, sustainSlider.getSlider()),
          releaseAttachment(processor.getApvts(), ParameterIDs::ampRelease, releaseSlider.getSlider())
    {
        addAndMakeVisible(visualizer);
        addAndMakeVisible(attackSlider);
        addAndMakeVisible(decaySlider);
        addAndMakeVisible(sustainSlider);
        addAndMakeVisible(releaseSlider);

        attackSlider .attachRandomizer(processor, ParameterIDs::ampAttack);
        decaySlider  .attachRandomizer(processor, ParameterIDs::ampDecay);
        sustainSlider.attachRandomizer(processor, ParameterIDs::ampSustain);
        releaseSlider.attachRandomizer(processor, ParameterIDs::ampRelease);
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

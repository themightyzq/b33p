#include "AmpEnvSection.h"

#include "Core/ParameterIDs.h"

namespace B33p
{
    AmpEnvSection::AmpEnvSection(juce::AudioProcessorValueTreeState& apvts)
        : Section("Amp Envelope"),
          visualizer(apvts),
          attackAttachment (apvts, ParameterIDs::ampAttack,  attackSlider.getSlider()),
          decayAttachment  (apvts, ParameterIDs::ampDecay,   decaySlider.getSlider()),
          sustainAttachment(apvts, ParameterIDs::ampSustain, sustainSlider.getSlider()),
          releaseAttachment(apvts, ParameterIDs::ampRelease, releaseSlider.getSlider())
    {
        addAndMakeVisible(visualizer);
        addAndMakeVisible(attackSlider);
        addAndMakeVisible(decaySlider);
        addAndMakeVisible(sustainSlider);
        addAndMakeVisible(releaseSlider);
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

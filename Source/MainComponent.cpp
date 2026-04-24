#include "MainComponent.h"

namespace B33p
{
    namespace
    {
        constexpr int kOuterPadding  = 12;
        constexpr int kGap           = 8;
        constexpr int kTopRowHeight  = 260;
        constexpr int kMidRowHeight  = 180;
    }

    MainComponent::MainComponent(B33pProcessor& processor)
        : oscillatorSection   (processor.getApvts()),
          ampEnvelopeSection  (processor.getApvts()),
          filterSection       (processor.getApvts()),
          effectsSection      (processor.getApvts()),
          masterSection       (processor),
          pitchEnvelopeSection(processor)
    {
        addAndMakeVisible(oscillatorSection);
        addAndMakeVisible(ampEnvelopeSection);
        addAndMakeVisible(filterSection);
        addAndMakeVisible(effectsSection);
        addAndMakeVisible(masterSection);
        addAndMakeVisible(pitchEnvelopeSection);

        setSize(900, 660);
    }

    void MainComponent::paint(juce::Graphics& g)
    {
        g.fillAll(juce::Colour::fromRGB(22, 22, 22));
    }

    void MainComponent::resized()
    {
        auto bounds = getLocalBounds().reduced(kOuterPadding);

        auto topRow = bounds.removeFromTop(kTopRowHeight);
        bounds.removeFromTop(kGap);
        auto midRow = bounds.removeFromTop(kMidRowHeight);
        bounds.removeFromTop(kGap);
        auto bottomRow = bounds;

        const int topCellWidth = (topRow.getWidth() - 2 * kGap) / 3;
        oscillatorSection.setBounds(topRow.removeFromLeft(topCellWidth));
        topRow.removeFromLeft(kGap);
        ampEnvelopeSection.setBounds(topRow.removeFromLeft(topCellWidth));
        topRow.removeFromLeft(kGap);
        filterSection.setBounds(topRow);

        const int midCellWidth = (midRow.getWidth() - kGap) / 2;
        effectsSection.setBounds(midRow.removeFromLeft(midCellWidth));
        midRow.removeFromLeft(kGap);
        masterSection.setBounds(midRow);

        pitchEnvelopeSection.setBounds(bottomRow);
    }
}

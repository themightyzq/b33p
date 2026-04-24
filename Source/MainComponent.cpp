#include "MainComponent.h"

namespace B33p
{
    namespace
    {
        constexpr int kOuterPadding = 12;
        constexpr int kGap          = 8;
        constexpr int kTopRowHeight = 260;
    }

    MainComponent::MainComponent(B33pProcessor& processor)
        : oscillatorSection (processor.getApvts()),
          ampEnvelopeSection(processor.getApvts()),
          filterSection     (processor.getApvts()),
          effectsSection    (processor.getApvts()),
          masterSection     (processor.getApvts())
    {
        addAndMakeVisible(oscillatorSection);
        addAndMakeVisible(ampEnvelopeSection);
        addAndMakeVisible(filterSection);
        addAndMakeVisible(effectsSection);
        addAndMakeVisible(masterSection);

        setSize(900, 520);
    }

    void MainComponent::paint(juce::Graphics& g)
    {
        g.fillAll(juce::Colour::fromRGB(22, 22, 22));
    }

    void MainComponent::resized()
    {
        auto bounds = getLocalBounds().reduced(kOuterPadding);

        auto topRow    = bounds.removeFromTop(kTopRowHeight);
        bounds.removeFromTop(kGap);
        auto bottomRow = bounds;

        const int topCellWidth = (topRow.getWidth() - 2 * kGap) / 3;
        oscillatorSection.setBounds(topRow.removeFromLeft(topCellWidth));
        topRow.removeFromLeft(kGap);
        ampEnvelopeSection.setBounds(topRow.removeFromLeft(topCellWidth));
        topRow.removeFromLeft(kGap);
        filterSection.setBounds(topRow);

        const int bottomCellWidth = (bottomRow.getWidth() - kGap) / 2;
        effectsSection.setBounds(bottomRow.removeFromLeft(bottomCellWidth));
        bottomRow.removeFromLeft(kGap);
        masterSection.setBounds(bottomRow);
    }
}

#include "MainComponent.h"

namespace B33p
{
    MainComponent::MainComponent()
    {
        setSize(600, 400);
    }

    void MainComponent::paint(juce::Graphics& g)
    {
        g.fillAll(juce::Colour::fromRGB(26, 26, 26));

        auto bounds = getLocalBounds();
        auto titleArea = bounds.removeFromTop(bounds.getHeight() * 3 / 5);

        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(64.0f));
        g.drawText("B33p", titleArea, juce::Justification::centredBottom);

        g.setColour(juce::Colours::grey);
        g.setFont(juce::FontOptions(14.0f));
        g.drawText("v" B33P_VERSION_STRING, bounds, juce::Justification::centredTop);
    }

    void MainComponent::resized() {}
}

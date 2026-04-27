#include "RandomizerWiring.h"

namespace B33p
{
    void wireRandomizerButtons(B33pProcessor& processor,
                               juce::Button& diceButton,
                               juce::Button& lockButton,
                               const juce::String& parameterID)
    {
        auto& randomizer = processor.getRandomizer();

        lockButton.setClickingTogglesState(true);
        lockButton.setToggleState(randomizer.isLocked(parameterID),
                                  juce::dontSendNotification);

        diceButton.onClick = [&processor, parameterID]
        {
            juce::Random rng;
            processor.getRandomizer().rollOne(parameterID, rng);
        };

        lockButton.onClick = [&processor, parameterID, &lockButton]
        {
            processor.getRandomizer().setLocked(parameterID,
                                                lockButton.getToggleState());
            processor.markDirty();
        };
    }
}

#pragma once

#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    // Shared wiring between a (dice, lock) button pair and the
    // processor's ParameterRandomizer for a given APVTS parameter ID.
    //
    //   - lockButton becomes a toggle button whose current state
    //     mirrors the randomizer's locked set; the initial visual
    //     state is populated from the randomizer.
    //   - diceButton.onClick rolls the parameter (no-op if locked).
    //   - lockButton.onClick pushes the new toggle state into the
    //     randomizer so the rest of the app sees it.
    //
    // Called once after both buttons are parented so JUCE has them
    // in the component hierarchy. Safe to call before the sliders
    // themselves are attached to APVTS.
    void wireRandomizerButtons(B33pProcessor& processor,
                               juce::Button& diceButton,
                               juce::Button& lockButton,
                               const juce::String& parameterID);
}

#pragma once

#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    // Compact rotary slider with a name label above, JUCE's default
    // value text box below the knob, and a dice + lock button pair
    // at the very bottom. Designed to drop into a section's inner
    // grid as a fixed-size cell.
    //
    // The slider is wired to an APVTS parameter by the owning
    // section via SliderAttachment. The dice and lock buttons are
    // wired to the ParameterRandomizer by calling attachRandomizer()
    // once the LabeledSlider is parented.
    class LabeledSlider : public juce::Component
    {
    public:
        explicit LabeledSlider(const juce::String& name);

        juce::Slider& getSlider() { return slider; }

        // Connects the dice + lock buttons to the processor's
        // randomizer for the given parameter ID. Safe to call before
        // any SliderAttachment is created — it only touches the
        // button state and the Randomizer's lock set.
        void attachRandomizer(B33pProcessor& processor,
                              const juce::String& parameterID);

        void resized() override;

    private:
        juce::Slider     slider;
        juce::Label      label;
        juce::TextButton diceButton { "D" };
        juce::TextButton lockButton { "L" };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabeledSlider)
    };
}

#pragma once

#include "IconButton.h"
#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    // juce::Slider with b33p's interaction niceties (REVIEW.md P9/P10/P25/P36):
    //   * hold Shift before dragging for fine adjustment
    //   * mouse-wheel / two-finger scroll steps the value
    //   * right-click (or ctrl-click) opens a menu: Enter value / Reset to
    //     default
    // The flat rotary visual lives in B33pLookAndFeel; this only adds behaviour.
    class B33pSlider : public juce::Slider
    {
    public:
        B33pSlider();
        void mouseDown(const juce::MouseEvent&) override;

    private:
        void showContextMenu();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(B33pSlider)
    };

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
        // showRandomizer = false suppresses the dice + lock buttons
        // (and their layout footprint). Used for parameters that
        // shouldn't be randomized at all — currently the master
        // gain, where a sudden +20 dB jump would blow ears out.
        explicit LabeledSlider(const juce::String& name,
                               bool showRandomizer = true);

        juce::Slider& getSlider() { return slider; }

        // Connects the dice + lock buttons to the processor's
        // randomizer for the given parameter ID. No-op when the
        // slider was constructed with showRandomizer = false.
        void attachRandomizer(B33pProcessor& processor,
                              const juce::String& parameterID);

        // Sets the same hover-tooltip on the knob and its label so
        // the user gets the hint regardless of where their cursor
        // lands. Dice + lock keep their own generic tooltips.
        void setTooltip(const juce::String& text);

        // Replaces the label text. Used by sections whose slider
        // semantics change at runtime (e.g. ModEffectsSection
        // re-labelling P1 between "Rate" / "Size" / "Time" per
        // effect type).
        void setLabelText(const juce::String& newLabelText);

        void resized() override;

    private:
        B33pSlider   slider;
        juce::Label  label;
        IconButton   diceButton { IconButton::Glyph::Die  };
        IconButton   lockButton { IconButton::Glyph::Lock };
        bool         randomizerVisible { true };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabeledSlider)
    };
}

#pragma once

#include "IconButton.h"
#include "State/B33pProcessor.h"
#include "State/ParameterRandomizer.h"

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
    //
    // Inherits ParameterRandomizer::RollListener (P35) so randomize
    // events for this slider's parameter trigger a brief change-flash
    // halo painted by `B33pLookAndFeel::drawRotarySlider` — gives the
    // user visual feedback for which knobs moved when they hit
    // Randomize.
    class LabeledSlider : public juce::Component
                        , private juce::Timer
                        , private ParameterRandomizer::RollListener
    {
    public:
        // showRandomizer = false suppresses the dice + lock buttons
        // (and their layout footprint). Used for parameters that
        // shouldn't be randomized at all — currently the master
        // gain, where a sudden +20 dB jump would blow ears out.
        explicit LabeledSlider(const juce::String& name,
                               bool showRandomizer = true);
        ~LabeledSlider() override;

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

        // Drives the modulation-glow halo painted by `B33pLookAndFeel::
        // drawRotarySlider`. The owning section's timer calls this each
        // tick with the current modulation intensity (0..1) for this
        // knob's destination — matrix LFO routings on the selected lane
        // for now, envelopes / Mod FX in a follow-up. The glow alpha
        // tracks the value; a zero / unchanged value short-circuits the
        // repaint so quiescent knobs cost nothing.
        void setModulationIntensity(float intensity01);

        void resized() override;

    private:
        // ParameterRandomizer::RollListener — matches `parameterID`
        // against `attachedParamId` and kicks the change-flash timer
        // on a match.
        void parameterRolled(const juce::String& parameterID) override;
        // juce::Timer — drives the change-flash decay. Only runs while
        // a flash is fading; stops itself when alpha reaches zero so
        // an idle synth still has no animation (accessibility rule).
        void timerCallback() override;

        B33pSlider   slider;
        juce::Label  label;
        IconButton   diceButton { IconButton::Glyph::Die  };
        IconButton   lockButton { IconButton::Glyph::Lock };
        bool         randomizerVisible { true };
        float        lastModulationIntensity { 0.0f };

        // P35 change-flash bookkeeping. attachedParamId is set by
        // attachRandomizer; rollListenerRegisteredWith is the randomizer
        // the slider is currently registered with (for un-registering
        // on the next attach or in the destructor).
        juce::String         attachedParamId;
        ParameterRandomizer* rollListenerRegisteredWith { nullptr };
        juce::int64          flashStartMs               { 0 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabeledSlider)
    };
}

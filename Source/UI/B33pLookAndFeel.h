#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    // b33p's house LookAndFeel — a flat, minimal, dark treatment (REVIEW.md
    // P8 / P15 / P29 / P30). Knobs lose JUCE's default "cricket-leg" line for
    // a flat body + thin value arc + dot; comboboxes, buttons, and popup menus
    // match the charcoal palette. The accent colour for a control's value arc
    // comes from its own colour IDs (rotarySliderFillColourId / trackColourId),
    // which the per-lane tint sets — so knobs take on the selected lane's
    // colour the same way the section accent strips do.
    class B33pLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        B33pLookAndFeel();

        void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                              float sliderPosProportional,
                              float rotaryStartAngle, float rotaryEndAngle,
                              juce::Slider&) override;

        void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                              float sliderPos, float minSliderPos, float maxSliderPos,
                              juce::Slider::SliderStyle, juce::Slider&) override;

        void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                          int buttonX, int buttonY, int buttonW, int buttonH,
                          juce::ComboBox&) override;

        void drawButtonBackground(juce::Graphics&, juce::Button&,
                                  const juce::Colour& backgroundColour,
                                  bool shouldDrawButtonAsHighlighted,
                                  bool shouldDrawButtonAsDown) override;

        void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;

    private:
        // A control is treated as bipolar (value arc / fill grows from the
        // centre) when its range is symmetric about zero — e.g. a ±semitone
        // pitch offset or a ±1 modulation amount.
        static bool isBipolar(const juce::Slider&);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(B33pLookAndFeel)
    };
}

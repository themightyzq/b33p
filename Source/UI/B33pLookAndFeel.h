#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <zqsfx_ui/zqsfx_ui.h>

namespace B33p
{
    // b33p's house LookAndFeel — a thin subclass of the shared zqsfx::ui::LookAndFeel
    // (docs/ZQSFX_UI_STYLE_GUIDE.md). The house LookAndFeel supplies rotary knobs (CC0
    // filmstrips, picked by dial size), combo boxes (LCD dropdowns), buttons (btn
    // gradient off-state / accent fill on-state), popup menus, and slider text-box
    // readouts (LCD glass + glow) automatically once drawRotarySlider / drawComboBox /
    // drawButtonBackground / drawLabel are left un-overridden.
    //
    // b33p is architecturally different from the other ZQ SFX products migrated so
    // far: its LookAndFeel deliberately does not own a palette. A control's accent
    // used to come from its own colour IDs (rotarySliderFillColourId / trackColourId),
    // set per section/lane by Section.cpp's tintSliders, so knobs took on the
    // selected lane's colour. That per-lane colour ID mechanism is PRESERVED — the
    // house filmstrip rotary knob does not consult it (it always draws the same knob
    // art), so the lane meaning colour now lives on LabeledSlider's label chip and on
    // Section's accent strip instead of the knob face (style guide: "never colour
    // alone" + "the house accent orange means active only, never data"). The one
    // bespoke slider style the house has no equivalent for — LinearHorizontal, used
    // by the modulation-matrix amount sliders and the pattern randomize-scope slider
    // — keeps a custom, restyled drawLinearSlider that still reads the per-control
    // colour ID, so those controls keep taking the lane's colour.
    class B33pLookAndFeel : public zqsfx::ui::LookAndFeel
    {
    public:
        B33pLookAndFeel();

        // Kept: the house LookAndFeel has no linear-slider equivalent. Restyled with
        // house tokens (hard-edged rectangle track, no rounded pill) for
        // LinearHorizontal; every other slider style falls back to
        // juce::LookAndFeel_V4 (matches pre-migration behaviour for those styles,
        // which were never drawn by this class to begin with).
        void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                              float sliderPos, float minSliderPos, float maxSliderPos,
                              juce::Slider::SliderStyle, juce::Slider&) override;

        // Numeric popup bubble shown while dragging a slider (house has no override
        // for this). VT323 via the house lcdFont so digits don't shift width mid-drag
        // — same rule as every other on-screen readout.
        juce::Font getSliderPopupFont(juce::Slider&) override;

    private:
        // A control is treated as bipolar (fill grows from the centre) when its
        // range is symmetric about zero — e.g. a modulation-matrix amount slider.
        static bool isBipolar(const juce::Slider&);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(B33pLookAndFeel)
    };
}

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    // Compact rotary slider with a name label above and JUCE's default
    // value text box below the knob. Designed to drop into a section's
    // inner grid as a fixed-size cell.
    //
    // The owner attaches the slider to an APVTS parameter via
    // SliderAttachment after construction; the slider's range is
    // populated by the attachment.
    class LabeledSlider : public juce::Component
    {
    public:
        explicit LabeledSlider(const juce::String& name);

        juce::Slider& getSlider() { return slider; }

        void resized() override;

    private:
        juce::Slider slider;
        juce::Label  label;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabeledSlider)
    };
}

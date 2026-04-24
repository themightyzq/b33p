#include "LabeledSlider.h"

namespace B33p
{
    namespace
    {
        constexpr int kLabelHeight    = 14;
        constexpr int kTextBoxHeight  = 16;
        constexpr int kTextBoxWidth   = 64;
    }

    LabeledSlider::LabeledSlider(const juce::String& name)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                               kTextBoxWidth, kTextBoxHeight);
        addAndMakeVisible(slider);

        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::FontOptions(11.0f));
        addAndMakeVisible(label);
    }

    void LabeledSlider::resized()
    {
        auto bounds = getLocalBounds();
        label.setBounds(bounds.removeFromTop(kLabelHeight));
        slider.setBounds(bounds);
    }
}

#include "LabeledSlider.h"

#include "RandomizerWiring.h"

namespace B33p
{
    namespace
    {
        constexpr int kLabelHeight      = 14;
        constexpr int kTextBoxHeight    = 16;
        constexpr int kTextBoxWidth     = 64;
        constexpr int kDiceLockHeight   = 20;
        constexpr int kDiceLockGap      = 4;
    }

    LabeledSlider::LabeledSlider(const juce::String& name, bool showRandomizer)
        : randomizerVisible(showRandomizer)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                               kTextBoxWidth, kTextBoxHeight);
        // While dragging, show the live formatted value in a popup
        // bubble next to the cursor — the small below-knob text box
        // is hard to read mid-gesture.
        slider.setPopupDisplayEnabled(true, false, nullptr);
        addAndMakeVisible(slider);

        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::FontOptions(11.0f));
        addAndMakeVisible(label);

        if (randomizerVisible)
        {
            addAndMakeVisible(diceButton);
            addAndMakeVisible(lockButton);
        }
    }

    void LabeledSlider::attachRandomizer(B33pProcessor& processor,
                                          const juce::String& parameterID)
    {
        if (! randomizerVisible)
            return;
        wireRandomizerButtons(processor, diceButton, lockButton, parameterID);
    }

    void LabeledSlider::setTooltip(const juce::String& text)
    {
        slider.setTooltip(text);
        label .setTooltip(text);
    }

    void LabeledSlider::setLabelText(const juce::String& newLabelText)
    {
        label.setText(newLabelText, juce::dontSendNotification);
    }

    void LabeledSlider::resized()
    {
        auto bounds = getLocalBounds();
        label.setBounds(bounds.removeFromTop(kLabelHeight));

        if (randomizerVisible)
        {
            auto diceLockRow = bounds.removeFromBottom(kDiceLockHeight);
            slider.setBounds(bounds);

            const int cellWidth = (diceLockRow.getWidth() - kDiceLockGap) / 2;
            diceButton.setBounds(diceLockRow.removeFromLeft(cellWidth));
            diceLockRow.removeFromLeft(kDiceLockGap);
            lockButton.setBounds(diceLockRow);
        }
        else
        {
            // No randomizer row — slider gets the whole remaining
            // height for its rotary visual.
            slider.setBounds(bounds);
        }
    }
}

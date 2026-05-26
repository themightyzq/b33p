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

    B33pSlider::B33pSlider()
    {
        setScrollWheelEnabled(true);     // P36: wheel / two-finger scroll steps
        setMouseDragSensitivity(250);    // default; Shift makes the drag finer
    }

    void B33pSlider::mouseDown(const juce::MouseEvent& e)
    {
        if (e.mods.isPopupMenu())        // P10: right-click / ctrl-click
        {
            showContextMenu();
            return;
        }

        // P9: hold Shift before dragging for ~6× finer control. Read at
        // mouse-down (the common "hold then drag" gesture); a higher
        // sensitivity means more travel per unit of value.
        setMouseDragSensitivity(e.mods.isShiftDown() ? 1500 : 250);
        juce::Slider::mouseDown(e);
    }

    void B33pSlider::showContextMenu()
    {
        juce::PopupMenu m;
        m.addItem(1, "Enter value...");                                   // P10 / P25
        m.addItem(2, "Reset to default", isDoubleClickReturnEnabled());   // P10

        m.showMenuAsync(
            juce::PopupMenu::Options().withTargetComponent(this),
            [safe = juce::Component::SafePointer<B33pSlider>(this)](int result)
            {
                if (auto* s = safe.getComponent())
                {
                    if (result == 1)
                        s->showTextBox();
                    else if (result == 2)
                        s->setValue(s->getDoubleClickReturnValue(), juce::sendNotification);
                }
            });
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
        // Bold the parameter name so it visually dominates the
        // dice + lock buttons sitting below the knob. Same 11 pt
        // size as before for consistency with other small labels;
        // weight is the differentiator.
        label.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
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
        // Wire double-click / "Reset to default" to the parameter's default,
        // in the slider's own value space (the SliderAttachment has already
        // matched the slider range to the parameter range). Done before the
        // randomizer-visibility early-out so even non-randomizable knobs reset.
        if (auto* p = processor.getApvts().getParameter(parameterID))
            slider.setDoubleClickReturnValue(true,
                (double) p->convertFrom0to1(p->getDefaultValue()));

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

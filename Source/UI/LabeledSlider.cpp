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

        // (Re-)register as a RollListener so randomize events on this
        // parameter trigger the change-flash. Re-attaching to a new lane
        // unhooks the previous registration first so we don't double-fire.
        auto& randomizer = processor.getRandomizer();
        if (rollListenerRegisteredWith != nullptr
            && rollListenerRegisteredWith != &randomizer)
            rollListenerRegisteredWith->removeRollListener(this);
        if (rollListenerRegisteredWith != &randomizer)
        {
            randomizer.addRollListener(this);
            rollListenerRegisteredWith = &randomizer;
        }
        attachedParamId = parameterID;

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

    LabeledSlider::~LabeledSlider()
    {
        if (rollListenerRegisteredWith != nullptr)
            rollListenerRegisteredWith->removeRollListener(this);
    }

    void LabeledSlider::parameterRolled(const juce::String& parameterID)
    {
        // Only flash on a match: every roll fires this for every
        // attached slider, and the broadcaster doesn't pre-filter.
        if (parameterID != attachedParamId)
            return;

        flashStartMs = juce::Time::currentTimeMillis();
        slider.getProperties().set("changeFlashAlpha", 1.0f);
        slider.repaint();
        if (! isTimerRunning())
            startTimerHz(30);
    }

    void LabeledSlider::timerCallback()
    {
        // ~800 ms ease-out decay so the eye catches the flash without
        // it lingering. The timer stops itself when the flash hits zero
        // so an idle synth has no animation (accessibility rule).
        constexpr juce::int64 kFlashDurationMs = 800;
        const auto elapsed = juce::Time::currentTimeMillis() - flashStartMs;
        if (elapsed >= kFlashDurationMs)
        {
            slider.getProperties().set("changeFlashAlpha", 0.0f);
            slider.repaint();
            stopTimer();
            return;
        }

        // Quadratic ease-out: bright at the start, fades quickly, lingers
        // briefly near zero — feels like a transient pop, not a continuous
        // pulse (which would read as "still doing something").
        const float t      = static_cast<float>(elapsed) / kFlashDurationMs;
        const float alpha  = (1.0f - t) * (1.0f - t);
        slider.getProperties().set("changeFlashAlpha", alpha);
        slider.repaint();
    }

    void LabeledSlider::setModulationIntensity(float intensity01)
    {
        const float clamped = juce::jlimit(0.0f, 1.0f, intensity01);
        // Short-circuit when nothing visible changed — 30 Hz timers on
        // every modulatable knob would otherwise repaint constantly even
        // when no LFO is routed and the value would round to the same
        // pixel anyway. The threshold matches drawRotarySlider's gate.
        if (std::abs(clamped - lastModulationIntensity) < 0.005f
            && clamped < 0.01f && lastModulationIntensity < 0.01f)
            return;

        lastModulationIntensity = clamped;
        slider.getProperties().set("modulationIntensity", clamped);
        slider.repaint();
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

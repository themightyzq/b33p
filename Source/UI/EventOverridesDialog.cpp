#include "EventOverridesDialog.h"

namespace B33p
{
    namespace
    {
        const juce::StringArray kDestNames {
            "None",
            "Osc Pitch",
            "Wavetable Morph",
            "FM Depth",
            "Ring Mix",
            "Filter Cutoff",
            "Filter Resonance",
            "Distortion Drive",
            "Mod FX P1",
            "Mod FX P2",
            "Mod FX Mix",
            "Voice Gain"
        };

        constexpr int kRowHeight        = 28;
        constexpr int kRowGap           = 4;
        constexpr int kHeaderHeight     = 22;
        constexpr int kButtonRowHeight  = 28;
        constexpr int kSlotLabelWidth   = 56;
        constexpr int kDestWidth        = 160;
        constexpr int kSliderRightInset = 4;
        constexpr int kInnerGap         = 6;
        constexpr int kPadding          = 12;
    }

    EventOverridesDialog::EventOverridesDialog(const Event& event,
                                                OnApply onApply,
                                                std::function<void()> onClose)
        : editing(event.overrides),
          onApplyCallback(std::move(onApply)),
          onCloseCallback(std::move(onClose))
    {
        for (int i = 0; i < kNumEventOverrides; ++i)
        {
            auto& slot = slotControls[static_cast<size_t>(i)];
            slot.label.setText("Slot " + juce::String(i + 1),
                                juce::dontSendNotification);
            slot.label.setJustificationType(juce::Justification::centredLeft);
            slot.label.setFont(juce::FontOptions(11.0f));
            addAndMakeVisible(slot.label);

            slot.dest.addItemList(kDestNames, 1);
            slot.dest.setSelectedId(static_cast<int>(editing[static_cast<size_t>(i)].destination) + 1,
                                     juce::dontSendNotification);
            slot.dest.onChange = [this, i]
            {
                const int sel = slotControls[static_cast<size_t>(i)].dest.getSelectedId() - 1;
                editing[static_cast<size_t>(i)].destination =
                    static_cast<ModDestination>(juce::jlimit(0, kNumModDestinations - 1, sel));
            };
            addAndMakeVisible(slot.dest);

            slot.amount.setSliderStyle(juce::Slider::LinearHorizontal);
            slot.amount.setRange(0.0, 1.0, 0.001);
            slot.amount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 18);
            slot.amount.setValue(static_cast<double>(editing[static_cast<size_t>(i)].value),
                                  juce::dontSendNotification);
            slot.amount.onValueChange = [this, i]
            {
                editing[static_cast<size_t>(i)].value =
                    static_cast<float>(slotControls[static_cast<size_t>(i)].amount.getValue());
            };
            addAndMakeVisible(slot.amount);
        }

        applyButton.onClick = [this]
        {
            if (onApplyCallback)
                onApplyCallback(editing);
            if (onCloseCallback)
                onCloseCallback();
        };
        cancelButton.onClick = [this]
        {
            if (onCloseCallback)
                onCloseCallback();
        };
        addAndMakeVisible(applyButton);
        addAndMakeVisible(cancelButton);

        const int desiredHeight = kHeaderHeight
                                + (kRowHeight + kRowGap) * kNumEventOverrides
                                + kInnerGap
                                + kButtonRowHeight
                                + kPadding * 2;
        setSize(560, desiredHeight);
    }

    void EventOverridesDialog::paint(juce::Graphics& g)
    {
        g.fillAll(juce::Colour::fromRGB(28, 28, 28));

        g.setColour(juce::Colour::fromRGB(180, 180, 180));
        g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
        g.drawText("Per-event parameter overrides — pin parameters for this hit only",
                   getLocalBounds().reduced(kPadding).removeFromTop(kHeaderHeight),
                   juce::Justification::centredLeft);
    }

    void EventOverridesDialog::resized()
    {
        auto bounds = getLocalBounds().reduced(kPadding);
        bounds.removeFromTop(kHeaderHeight);

        for (int i = 0; i < kNumEventOverrides; ++i)
        {
            auto row = bounds.removeFromTop(kRowHeight);
            bounds.removeFromTop(kRowGap);

            auto& slot = slotControls[static_cast<size_t>(i)];
            slot.label.setBounds(row.removeFromLeft(kSlotLabelWidth));
            row.removeFromLeft(kInnerGap);
            slot.dest.setBounds(row.removeFromLeft(kDestWidth));
            row.removeFromLeft(kInnerGap);
            slot.amount.setBounds(row.withTrimmedRight(kSliderRightInset));
        }

        bounds.removeFromTop(kInnerGap);
        auto buttonRow = bounds.removeFromTop(kButtonRowHeight);
        constexpr int kButtonWidth = 100;
        applyButton.setBounds(buttonRow.removeFromRight(kButtonWidth));
        buttonRow.removeFromRight(kInnerGap);
        cancelButton.setBounds(buttonRow.removeFromRight(kButtonWidth));
    }

    EventOverridesDialogWindow::EventOverridesDialogWindow(
            const Event& event,
            EventOverridesDialog::OnApply onApply,
            std::function<void()> onClose)
        : DocumentWindow("Event Overrides",
                         juce::Colour::fromRGB(22, 22, 22),
                         DocumentWindow::closeButton),
          onCloseCallback(std::move(onClose))
    {
        setUsingNativeTitleBar(true);
        auto* dialog = new EventOverridesDialog(event,
                                                 std::move(onApply),
                                                 [this] { closeButtonPressed(); });
        setContentOwned(dialog, true);
        setResizable(false, false);
        centreWithSize(getWidth(), getHeight());
    }

    void EventOverridesDialogWindow::closeButtonPressed()
    {
        // Swap out the callback so the underlying dialog's own
        // close call doesn't re-enter this method recursively
        // when the apply path triggers it.
        auto cb = std::move(onCloseCallback);
        onCloseCallback = nullptr;
        if (cb)
            cb();
    }
}

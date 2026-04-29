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
        constexpr int kSectionGap       = 12;
        constexpr int kButtonRowHeight  = 28;
        constexpr int kSlotLabelWidth   = 56;
        constexpr int kDestWidth        = 160;
        constexpr int kSliderRightInset = 4;
        constexpr int kInnerGap         = 6;
        constexpr int kPadding          = 12;
        constexpr int kPropertyLabelW   = 96;
    }

    EventOverridesDialog::EventOverridesDialog(const Event& event,
                                                OnApply onApply,
                                                std::function<void()> onClose)
        : onApplyCallback(std::move(onApply)),
          onCloseCallback(std::move(onClose))
    {
        editing.overrides       = event.overrides;
        editing.probability     = event.probability;
        editing.ratchets        = event.ratchets;
        editing.humanizeAmount  = event.humanizeAmount;

        for (int i = 0; i < kNumEventOverrides; ++i)
        {
            auto& slot = slotControls[static_cast<size_t>(i)];
            slot.label.setText("Slot " + juce::String(i + 1),
                                juce::dontSendNotification);
            slot.label.setJustificationType(juce::Justification::centredLeft);
            slot.label.setFont(juce::FontOptions(11.0f));
            addAndMakeVisible(slot.label);

            slot.dest.addItemList(kDestNames, 1);
            slot.dest.setSelectedId(static_cast<int>(editing.overrides[static_cast<size_t>(i)].destination) + 1,
                                     juce::dontSendNotification);
            slot.dest.onChange = [this, i]
            {
                const int sel = slotControls[static_cast<size_t>(i)].dest.getSelectedId() - 1;
                editing.overrides[static_cast<size_t>(i)].destination =
                    static_cast<ModDestination>(juce::jlimit(0, kNumModDestinations - 1, sel));
            };
            addAndMakeVisible(slot.dest);

            slot.amount.setSliderStyle(juce::Slider::LinearHorizontal);
            slot.amount.setRange(0.0, 1.0, 0.001);
            slot.amount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 18);
            slot.amount.setValue(static_cast<double>(editing.overrides[static_cast<size_t>(i)].value),
                                  juce::dontSendNotification);
            slot.amount.onValueChange = [this, i]
            {
                editing.overrides[static_cast<size_t>(i)].value =
                    static_cast<float>(slotControls[static_cast<size_t>(i)].amount.getValue());
            };
            addAndMakeVisible(slot.amount);
        }

        // ---- Probability / ratchets / humanize ---------------------
        auto setupPropertyLabel = [&](juce::Label& l)
        {
            l.setJustificationType(juce::Justification::centredLeft);
            l.setFont(juce::FontOptions(11.0f));
            addAndMakeVisible(l);
        };
        setupPropertyLabel(probabilityLabel);
        setupPropertyLabel(ratchetsLabel);
        setupPropertyLabel(humanizeLabel);

        probabilitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
        probabilitySlider.setRange(0.0, 1.0, 0.001);
        probabilitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 18);
        probabilitySlider.setValue(static_cast<double>(editing.probability),
                                    juce::dontSendNotification);
        probabilitySlider.onValueChange = [this]
        {
            editing.probability = static_cast<float>(probabilitySlider.getValue());
        };
        probabilitySlider.setTooltip("0 = never fires, 1 = always fires (rolled at snapshot time)");
        addAndMakeVisible(probabilitySlider);

        ratchetsSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        ratchetsSlider.setRange(1.0, static_cast<double>(kMaxRatchets), 1.0);
        ratchetsSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 18);
        ratchetsSlider.setValue(static_cast<double>(editing.ratchets),
                                 juce::dontSendNotification);
        ratchetsSlider.onValueChange = [this]
        {
            editing.ratchets = juce::jlimit(1, kMaxRatchets,
                                             static_cast<int>(ratchetsSlider.getValue()));
        };
        ratchetsSlider.setTooltip("1 = single hit; higher = N evenly-spaced retriggers within the event's duration");
        addAndMakeVisible(ratchetsSlider);

        humanizeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        humanizeSlider.setRange(0.0, 1.0, 0.001);
        humanizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 18);
        humanizeSlider.setValue(static_cast<double>(editing.humanizeAmount),
                                 juce::dontSendNotification);
        humanizeSlider.onValueChange = [this]
        {
            editing.humanizeAmount = static_cast<float>(humanizeSlider.getValue());
        };
        humanizeSlider.setTooltip("Random jitter on timing + velocity. Re-randomises each snapshot rebuild.");
        addAndMakeVisible(humanizeSlider);

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

        const int desiredHeight = kPadding * 2
                                + kHeaderHeight
                                + (kRowHeight + kRowGap) * kNumEventOverrides
                                + kSectionGap
                                + (kRowHeight + kRowGap) * 3
                                + kInnerGap
                                + kButtonRowHeight;
        setSize(560, desiredHeight);
    }

    void EventOverridesDialog::paint(juce::Graphics& g)
    {
        g.fillAll(juce::Colour::fromRGB(28, 28, 28));

        g.setColour(juce::Colour::fromRGB(180, 180, 180));
        g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
        g.drawText("Event properties — overrides + probability / ratcheting / humanize",
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

        bounds.removeFromTop(kSectionGap);

        auto layoutPropertyRow = [&](juce::Label& label, juce::Slider& slider)
        {
            auto row = bounds.removeFromTop(kRowHeight);
            bounds.removeFromTop(kRowGap);
            label.setBounds(row.removeFromLeft(kPropertyLabelW));
            row.removeFromLeft(kInnerGap);
            slider.setBounds(row.withTrimmedRight(kSliderRightInset));
        };
        layoutPropertyRow(probabilityLabel, probabilitySlider);
        layoutPropertyRow(ratchetsLabel,    ratchetsSlider);
        layoutPropertyRow(humanizeLabel,    humanizeSlider);

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
        : DocumentWindow("Event Properties",
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

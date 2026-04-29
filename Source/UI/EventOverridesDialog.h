#pragma once

#include "DSP/ModulationMatrix.h"
#include "Pattern/Pattern.h"
#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace B33p
{
    // Bundles every per-event field the dialog edits: the array of
    // override slots plus probability / ratchets / humanize. Lets
    // the OnApply callback apply all edits in one Pattern mutation.
    struct EventDialogEdits
    {
        std::array<EventOverride, kNumEventOverrides> overrides;
        float probability    { 1.0f };
        int   ratchets       { 1 };
        float humanizeAmount { 0.0f };
    };

    // Modal-ish popup that edits an Event's per-hit properties:
    // the four override slots (destination + amount), plus
    // probability, ratcheting, and humanize. Reads the event on
    // open, fires the OnApply callback with the edited values when
    // the user clicks Apply. Cancel / closing the window dismisses
    // without applying.
    //
    // The dialog does not own the pattern — it simply produces an
    // edited bundle and hands it to the caller, who is expected to
    // wrap any pattern mutation in their existing undo machinery.
    class EventOverridesDialog : public juce::Component
    {
    public:
        using OnApply = std::function<void(const EventDialogEdits&)>;

        EventOverridesDialog(const Event& event,
                             OnApply onApply,
                             std::function<void()> onClose);

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        struct SlotControls
        {
            juce::Label    label;
            juce::ComboBox dest;
            juce::Slider   amount;
        };

        EventDialogEdits                          editing;
        std::array<SlotControls, kNumEventOverrides> slotControls;

        juce::Label  probabilityLabel { {}, "Probability" };
        juce::Slider probabilitySlider;
        juce::Label  ratchetsLabel    { {}, "Ratchets" };
        juce::Slider ratchetsSlider;
        juce::Label  humanizeLabel    { {}, "Humanize" };
        juce::Slider humanizeSlider;

        juce::TextButton applyButton  { "Apply"  };
        juce::TextButton cancelButton { "Cancel" };

        OnApply               onApplyCallback;
        std::function<void()> onCloseCallback;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EventOverridesDialog)
    };

    // Free-floating popup window that hosts the dialog. Closes by
    // calling its onClose handler (which the owner uses to drop
    // the unique_ptr and dispose of the window cleanly).
    class EventOverridesDialogWindow : public juce::DocumentWindow
    {
    public:
        EventOverridesDialogWindow(const Event& event,
                                    EventOverridesDialog::OnApply onApply,
                                    std::function<void()> onClose);

        void closeButtonPressed() override;

    private:
        std::function<void()> onCloseCallback;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EventOverridesDialogWindow)
    };
}

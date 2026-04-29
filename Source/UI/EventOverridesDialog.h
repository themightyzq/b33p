#pragma once

#include "DSP/ModulationMatrix.h"
#include "Pattern/Pattern.h"
#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace B33p
{
    // Modal-ish popup that edits the four EventOverride slots on a
    // single Pattern Event. Reads the event's current overrides on
    // open, lets the user pick a destination (combo) and value
    // (linear horizontal slider 0..1) per slot, and fires the
    // OnApply callback with the edited overrides when the user
    // clicks Apply. Cancel / closing the window dismisses without
    // applying.
    //
    // The dialog does not own the pattern — it simply produces an
    // edited overrides array and hands it to the caller, who is
    // expected to wrap any pattern mutation in their existing
    // undo machinery.
    class EventOverridesDialog : public juce::Component
    {
    public:
        using OnApply = std::function<void(const std::array<EventOverride,
                                                            kNumEventOverrides>&)>;

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

        std::array<EventOverride,  kNumEventOverrides> editing;
        std::array<SlotControls,   kNumEventOverrides> slotControls;

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

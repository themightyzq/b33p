#pragma once

#include "State/PresetManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

namespace B33p
{
    // Compact preset browser. Lists every `.beep` file in the
    // user's presets directory; the user picks one and clicks Load
    // (or double-clicks the row) to apply it. A small Delete button
    // removes the selected preset (with a confirm prompt).
    //
    // The dialog never directly mutates processor state: load /
    // delete decisions go back through callbacks the owner registers,
    // so the owner can drive its own discard-confirm flow + undo
    // bookkeeping before the actual file I/O happens.
    class PresetBrowserDialog : public juce::Component
                               , private juce::ListBoxModel
    {
    public:
        using OnLoad   = std::function<void(const juce::File&)>;
        using OnDelete = std::function<void(const juce::File&)>;

        PresetBrowserDialog(PresetManager& manager,
                            OnLoad onLoad,
                            OnDelete onDelete,
                            std::function<void()> onClose);

        void paint(juce::Graphics& g) override;
        void resized() override;

        // Repopulate the list — used after save / delete so the
        // browser stays in sync with the on-disk directory.
        void refresh();

    private:
        // juce::ListBoxModel
        int  getNumRows() override;
        void paintListBoxItem(int rowNumber,
                               juce::Graphics& g,
                               int width, int height,
                               bool rowIsSelected) override;
        void listBoxItemDoubleClicked(int row,
                                       const juce::MouseEvent& e) override;
        void selectedRowsChanged(int lastRowSelected) override;

        void requestLoadSelected();
        void requestDeleteSelected();

        PresetManager& manager;

        juce::ListBox list { {}, this };
        juce::TextButton loadButton   { "Load"   };
        juce::TextButton deleteButton { "Delete" };
        juce::TextButton closeButton  { "Close"  };

        std::vector<juce::File> presets;

        OnLoad                onLoadCallback;
        OnDelete              onDeleteCallback;
        std::function<void()> onCloseCallback;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserDialog)
    };

    // Free-floating popup window that hosts the dialog. Closes by
    // calling its onClose handler (the owner uses it to drop the
    // unique_ptr).
    class PresetBrowserDialogWindow : public juce::DocumentWindow
    {
    public:
        PresetBrowserDialogWindow(PresetManager& manager,
                                   PresetBrowserDialog::OnLoad onLoad,
                                   PresetBrowserDialog::OnDelete onDelete,
                                   std::function<void()> onClose);

        void closeButtonPressed() override;

        // Invalidates the browser's row list. Owner calls this
        // after Save Preset so a freshly-saved preset shows up
        // in the list immediately.
        void refresh();

    private:
        std::function<void()> onCloseCallback;
        PresetBrowserDialog*  dialog { nullptr };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserDialogWindow)
    };
}

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
        // Fired after a successful rename with (oldFile, newFile) so the
        // owner can follow a renamed preset if it was the currently loaded
        // one (its displayed name is derived from the file path, not
        // stored anywhere else).
        using OnRename = std::function<void(const juce::File&, const juce::File&)>;

        PresetBrowserDialog(PresetManager& manager,
                            OnLoad onLoad,
                            OnDelete onDelete,
                            OnRename onRename,
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
        void requestRenameSelected();

        // Re-selects the row matching f after refresh() has rebuilt the
        // list (e.g. after a rename), so the renamed preset stays the
        // active selection instead of dropping back to "nothing selected".
        void selectPresetFile(const juce::File& f);

        PresetManager& manager;

        juce::ListBox list { {}, this };
        juce::TextButton loadButton   { "Load"   };
        juce::TextButton deleteButton { "Delete" };
        juce::TextButton renameButton { "Rename" };
        juce::TextButton closeButton  { "Close"  };

        // Owns the Rename AlertWindow the same way MainComponent owns its
        // Save Preset one: enterModalState(deleteWhenDismissed=false) needs
        // somewhere non-transient to keep the object alive, and resetting
        // this is what actually deletes it.
        std::unique_ptr<juce::AlertWindow> renameWindow;

        // Shown when presets is empty so the user knows where to
        // start instead of staring at a blank list.
        juce::Label emptyStateLabel;

        std::vector<juce::File> presets;

        OnLoad                onLoadCallback;
        OnDelete              onDeleteCallback;
        OnRename              onRenameCallback;
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
                                   PresetBrowserDialog::OnRename onRename,
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

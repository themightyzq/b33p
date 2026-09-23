#include "PresetBrowserDialog.h"

#include <zqsfx_ui/zqsfx_ui.h>

namespace B33p
{
    namespace
    {
        constexpr int kRowHeight       = 22;
        constexpr int kPadding         = 12;
        constexpr int kButtonRowHeight = 28;
        constexpr int kButtonGap       = 6;
        constexpr int kButtonWidth     = 90;
        constexpr int kHeaderHeight    = 22;
    }

    PresetBrowserDialog::PresetBrowserDialog(PresetManager& managerRef,
                                              OnLoad onLoad,
                                              OnDelete onDelete,
                                              OnRename onRename,
                                              std::function<void()> onClose)
        : manager(managerRef),
          onLoadCallback(std::move(onLoad)),
          onDeleteCallback(std::move(onDelete)),
          onRenameCallback(std::move(onRename)),
          onCloseCallback(std::move(onClose))
    {
        setAccessible(true);
        setTitle("Preset browser");
        setDescription("Browse, load, and delete saved presets");

        list.setRowHeight(kRowHeight);
        list.setMultipleSelectionEnabled(false);
        list.setTitle("Preset list");
        addAndMakeVisible(list);

        loadButton.onClick   = [this] { requestLoadSelected();   };
        deleteButton.onClick = [this] { requestDeleteSelected(); };
        renameButton.onClick = [this] { requestRenameSelected(); };
        closeButton.onClick  = [this]
        {
            if (onCloseCallback)
                onCloseCallback();
        };
        loadButton.setTitle("Load selected preset");
        deleteButton.setTitle("Delete selected preset");
        renameButton.setTitle("Rename selected preset");
        renameButton.setDescription(
            "Rename the selected user preset. Factory presets can't be renamed.");
        renameButton.setTooltip("Rename the selected preset");
        closeButton.setTitle("Close preset browser");
        addAndMakeVisible(loadButton);
        addAndMakeVisible(deleteButton);
        addAndMakeVisible(renameButton);
        addAndMakeVisible(closeButton);

        // Buttons that need a selection start disabled — they enable
        // when the user clicks a row in selectedRowsChanged.
        loadButton.setEnabled(false);
        deleteButton.setEnabled(false);
        renameButton.setEnabled(false);

        // Empty-state hint overlay (visibility toggled in refresh()).
        emptyStateLabel.setText(
            "No presets yet.\n\nUse File > Save Preset... to save the current patch.",
            juce::dontSendNotification);
        emptyStateLabel.setJustificationType(juce::Justification::centred);
        emptyStateLabel.setFont(juce::FontOptions(12.0f).withStyle("Italic"));
        emptyStateLabel.setColour(juce::Label::textColourId,
                                    zqsfx::ui::colour::silkCaption);
        emptyStateLabel.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(emptyStateLabel);

        refresh();
        setSize(420, 360);
    }

    void PresetBrowserDialog::refresh()
    {
        presets = manager.listPresets();
        list.updateContent();
        list.deselectAllRows();
        loadButton.setEnabled(false);
        deleteButton.setEnabled(false);
        emptyStateLabel.setVisible(presets.empty());
    }

    int PresetBrowserDialog::getNumRows()
    {
        return static_cast<int>(presets.size());
    }

    void PresetBrowserDialog::paintListBoxItem(int rowNumber,
                                                juce::Graphics& g,
                                                int width, int height,
                                                bool rowIsSelected)
    {
        if (rowNumber < 0 || rowNumber >= static_cast<int>(presets.size()))
            return;

        if (rowIsSelected)
        {
            // Selection = the one focused/active row -> house accent, per the
            // style guide's "active toggle is an accent fill" rule. Row text
            // switches to accentInk for contrast against the fill.
            g.setColour(zqsfx::ui::colour::accent);
            g.fillRect(0, 0, width, height);
        }

        const auto& f = presets[static_cast<size_t>(rowNumber)];
        const auto displayName = f.getFileNameWithoutExtension();

        // Factory presets ship with a "Factory - " prefix (see
        // GeneratorPresets.cpp). Render them italic (a shape cue, not just a
        // colour one) so the user can scan factory vs user presets at a
        // glance — keeps the factory set from disappearing into the
        // user-saved list as the directory grows. Both colours below are
        // neutral house tokens rather than a lane channel, since this
        // distinction has nothing to do with lane identity.
        const bool isFactory = PresetManager::isFactoryPresetName(displayName);
        if (rowIsSelected)
            g.setColour(zqsfx::ui::colour::accentInk);
        else
            g.setColour(isFactory ? zqsfx::ui::colour::silkCaption : zqsfx::ui::colour::btnText);
        g.setFont(juce::FontOptions(13.0f, isFactory ? juce::Font::italic : juce::Font::plain));

        g.drawText(displayName,
                   juce::Rectangle<int>(8, 0, width - 16, height),
                   juce::Justification::centredLeft);
    }

    void PresetBrowserDialog::listBoxItemDoubleClicked(int /*row*/,
                                                        const juce::MouseEvent&)
    {
        requestLoadSelected();
    }

    void PresetBrowserDialog::selectedRowsChanged(int /*lastRowSelected*/)
    {
        const int row = list.getSelectedRow();
        const bool hasSelection = row >= 0 && row < static_cast<int>(presets.size());
        loadButton.setEnabled(hasSelection);
        deleteButton.setEnabled(hasSelection);

        bool canRename = false;
        if (hasSelection)
        {
            const auto& f = presets[static_cast<size_t>(row)];
            canRename = ! PresetManager::isFactoryPresetName(f.getFileNameWithoutExtension());
        }
        renameButton.setEnabled(canRename);
        renameButton.setTooltip(hasSelection && ! canRename
            ? "Factory presets can't be renamed. Save a copy under a new name instead."
            : "Rename the selected preset");
    }

    void PresetBrowserDialog::requestLoadSelected()
    {
        const int row = list.getSelectedRow();
        if (row < 0 || row >= static_cast<int>(presets.size()))
            return;
        if (onLoadCallback)
            onLoadCallback(presets[static_cast<size_t>(row)]);
    }

    void PresetBrowserDialog::requestDeleteSelected()
    {
        const int row = list.getSelectedRow();
        if (row < 0 || row >= static_cast<int>(presets.size()))
            return;

        const auto file = presets[static_cast<size_t>(row)];

        // Confirm because preset deletion is irreversible (no
        // trash / recycle integration on macOS / Windows).
        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::WarningIcon)
                .withTitle("Delete preset")
                .withMessage("Permanently delete \""
                              + file.getFileNameWithoutExtension() + "\"?")
                .withButton("Delete")
                .withButton("Cancel"),
            [this, file](int result)
            {
                if (result == 1 && onDeleteCallback)
                    onDeleteCallback(file);
            });
    }

    void PresetBrowserDialog::requestRenameSelected()
    {
        const int row = list.getSelectedRow();
        if (row < 0 || row >= static_cast<int>(presets.size()))
            return;

        const auto file = presets[static_cast<size_t>(row)];
        // Defensive: the button is disabled for factory rows, but don't
        // trust UI state alone for something that touches disk.
        if (PresetManager::isFactoryPresetName(file.getFileNameWithoutExtension()))
            return;

        auto* aw = new juce::AlertWindow("Rename Preset",
                                          "Enter a new name for \""
                                              + file.getFileNameWithoutExtension() + "\":",
                                          juce::MessageBoxIconType::QuestionIcon);
        aw->addTextEditor("name", file.getFileNameWithoutExtension(), "Preset name");
        aw->addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey));
        aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        renameWindow.reset(aw);
        aw->enterModalState(true,
            juce::ModalCallbackFunction::create(
                [this, aw, file](int result)
                {
                    const juce::String newName = aw->getTextEditorContents("name");
                    renameWindow.reset();   // also deletes aw

                    if (result != 1)
                        return;

                    const auto reason = manager.validateNewPresetName(newName, file);
                    if (reason.isNotEmpty())
                    {
                        juce::AlertWindow::showAsync(
                            juce::MessageBoxOptions()
                                .withIconType(juce::MessageBoxIconType::WarningIcon)
                                .withTitle("Rename Preset failed")
                                .withMessage(reason)
                                .withButton("OK"),
                            nullptr);
                        return;
                    }

                    const auto renamed = manager.renamePreset(file, newName);
                    if (renamed == juce::File())
                    {
                        juce::AlertWindow::showAsync(
                            juce::MessageBoxOptions()
                                .withIconType(juce::MessageBoxIconType::WarningIcon)
                                .withTitle("Rename Preset failed")
                                .withMessage("Could not rename the preset file on disk.")
                                .withButton("OK"),
                            nullptr);
                        return;
                    }

                    refresh();
                    selectPresetFile(renamed);
                    if (onRenameCallback)
                        onRenameCallback(file, renamed);
                }),
            false);   // we own the AlertWindow via renameWindow; don't let
                       // the modal helper delete it out from under us
    }

    void PresetBrowserDialog::selectPresetFile(const juce::File& f)
    {
        for (size_t i = 0; i < presets.size(); ++i)
        {
            if (presets[i] == f)
            {
                list.selectRow(static_cast<int>(i));
                return;
            }
        }
    }

    void PresetBrowserDialog::paint(juce::Graphics& g)
    {
        g.fillAll(zqsfx::ui::colour::panelBot);

        g.setColour(zqsfx::ui::colour::silkTitle);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText("Presets - double-click a row to load",
                   getLocalBounds().reduced(kPadding).removeFromTop(kHeaderHeight),
                   juce::Justification::centredLeft);
    }

    void PresetBrowserDialog::resized()
    {
        auto bounds = getLocalBounds().reduced(kPadding);
        bounds.removeFromTop(kHeaderHeight);

        auto buttonRow = bounds.removeFromBottom(kButtonRowHeight);
        bounds.removeFromBottom(kButtonGap);
        list.setBounds(bounds);
        // Empty-state label sits over the same area as the list.
        // setInterceptsMouseClicks(false) was set on construction, so
        // it never blocks clicks even when visible.
        emptyStateLabel.setBounds(bounds);

        // Right-aligned: Close on the far right, then Delete, then
        // Rename, Load at the far left so the primary action sits where
        // the eye lands first.
        loadButton.setBounds(buttonRow.removeFromLeft(kButtonWidth));
        closeButton .setBounds(buttonRow.removeFromRight(kButtonWidth));
        buttonRow.removeFromRight(kButtonGap);
        deleteButton.setBounds(buttonRow.removeFromRight(kButtonWidth));
        buttonRow.removeFromRight(kButtonGap);
        renameButton.setBounds(buttonRow.removeFromRight(kButtonWidth));
    }

    PresetBrowserDialogWindow::PresetBrowserDialogWindow(
            PresetManager& manager,
            PresetBrowserDialog::OnLoad onLoad,
            PresetBrowserDialog::OnDelete onDelete,
            PresetBrowserDialog::OnRename onRename,
            std::function<void()> onClose)
        : DocumentWindow("Preset Browser",
                         zqsfx::ui::colour::chassisMid,
                         DocumentWindow::closeButton),
          onCloseCallback(std::move(onClose))
    {
        setUsingNativeTitleBar(true);
        auto* d = new PresetBrowserDialog(manager,
                                           std::move(onLoad),
                                           std::move(onDelete),
                                           std::move(onRename),
                                           [this] { closeButtonPressed(); });
        dialog = d;
        setContentOwned(d, true);
        setResizable(true, false);
        centreWithSize(getWidth(), getHeight());
    }

    void PresetBrowserDialogWindow::closeButtonPressed()
    {
        auto cb = std::move(onCloseCallback);
        onCloseCallback = nullptr;
        if (cb)
            cb();
    }

    void PresetBrowserDialogWindow::refresh()
    {
        if (dialog != nullptr)
            dialog->refresh();
    }
}

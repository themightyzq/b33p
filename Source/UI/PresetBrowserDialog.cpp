#include "PresetBrowserDialog.h"

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
                                              std::function<void()> onClose)
        : manager(managerRef),
          onLoadCallback(std::move(onLoad)),
          onDeleteCallback(std::move(onDelete)),
          onCloseCallback(std::move(onClose))
    {
        list.setRowHeight(kRowHeight);
        list.setMultipleSelectionEnabled(false);
        addAndMakeVisible(list);

        loadButton.onClick   = [this] { requestLoadSelected();   };
        deleteButton.onClick = [this] { requestDeleteSelected(); };
        closeButton.onClick  = [this]
        {
            if (onCloseCallback)
                onCloseCallback();
        };
        addAndMakeVisible(loadButton);
        addAndMakeVisible(deleteButton);
        addAndMakeVisible(closeButton);

        // Buttons that need a selection start disabled — they enable
        // when the user clicks a row in selectedRowsChanged.
        loadButton.setEnabled(false);
        deleteButton.setEnabled(false);

        // Empty-state hint overlay (visibility toggled in refresh()).
        emptyStateLabel.setText(
            "No presets yet.\n\nUse File ▸ Save Preset… to save the current patch.",
            juce::dontSendNotification);
        emptyStateLabel.setJustificationType(juce::Justification::centred);
        emptyStateLabel.setFont(juce::FontOptions(12.0f).withStyle("Italic"));
        emptyStateLabel.setColour(juce::Label::textColourId,
                                    juce::Colour::fromRGB(140, 140, 140));
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
            g.setColour(juce::Colour::fromRGB(60, 90, 130));
            g.fillRect(0, 0, width, height);
        }

        const auto& f = presets[static_cast<size_t>(rowNumber)];
        const auto displayName = f.getFileNameWithoutExtension();

        // Factory presets ship with a "Factory - " prefix (see
        // GeneratorPresets.cpp). Render them italic + slightly cooler
        // tint so the user can scan factory vs user presets at a
        // glance — keeps the factory set from disappearing into the
        // user-saved list as the directory grows.
        const bool isFactory = displayName.startsWith("Factory - ");
        if (isFactory)
        {
            g.setColour(juce::Colour::fromRGB(170, 185, 215));
            g.setFont(juce::FontOptions(13.0f).withStyle("Italic"));
        }
        else
        {
            g.setColour(juce::Colour::fromRGB(220, 220, 220));
            g.setFont(juce::FontOptions(13.0f));
        }

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
        const bool hasSelection = list.getSelectedRow() >= 0;
        loadButton.setEnabled(hasSelection);
        deleteButton.setEnabled(hasSelection);
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

    void PresetBrowserDialog::paint(juce::Graphics& g)
    {
        g.fillAll(juce::Colour::fromRGB(28, 28, 28));

        g.setColour(juce::Colour::fromRGB(180, 180, 180));
        g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
        g.drawText("Presets — double-click a row to load",
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

        // Right-aligned: Close on the far right, Delete left of it,
        // Load at the far left so the primary action sits where the
        // eye lands first.
        loadButton.setBounds(buttonRow.removeFromLeft(kButtonWidth));
        closeButton .setBounds(buttonRow.removeFromRight(kButtonWidth));
        buttonRow.removeFromRight(kButtonGap);
        deleteButton.setBounds(buttonRow.removeFromRight(kButtonWidth));
    }

    PresetBrowserDialogWindow::PresetBrowserDialogWindow(
            PresetManager& manager,
            PresetBrowserDialog::OnLoad onLoad,
            PresetBrowserDialog::OnDelete onDelete,
            std::function<void()> onClose)
        : DocumentWindow("Preset Browser",
                         juce::Colour::fromRGB(22, 22, 22),
                         DocumentWindow::closeButton),
          onCloseCallback(std::move(onClose))
    {
        setUsingNativeTitleBar(true);
        auto* d = new PresetBrowserDialog(manager,
                                           std::move(onLoad),
                                           std::move(onDelete),
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

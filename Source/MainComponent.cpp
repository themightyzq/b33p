#include "MainComponent.h"

namespace B33p
{
    namespace
    {
        constexpr int kOuterPadding   = 12;
        constexpr int kGap            = 8;
        constexpr int kTopRowHeight   = 260;
        constexpr int kMidRowHeight   = 180;
        constexpr int kPitchRowHeight = 180;
        constexpr int kMenuBarHeight  = 24;

        // Menu item IDs. Kept in one enum so the dispatch in
        // menuItemSelected stays readable. IDs must be > 0 — JUCE
        // treats 0 as "nothing selected".
        enum MenuId
        {
            FileOpen = 1,
            FileSave,
            FileSaveAs,
            EditUndo,
            EditRedo,
            HelpAbout,
        };
    }

    MainComponent::MainComponent(B33pProcessor& processorRef)
        : processor(processorRef),
          fileManager(processorRef),
          oscillatorSection   (processor),
          ampEnvelopeSection  (processor),
          filterSection       (processor),
          effectsSection      (processor),
          masterSection       (processor),
          pitchEnvelopeSection(processor),
          patternSection      (processor)
    {
        addAndMakeVisible(menuBar);
        addAndMakeVisible(oscillatorSection);
        addAndMakeVisible(ampEnvelopeSection);
        addAndMakeVisible(filterSection);
        addAndMakeVisible(effectsSection);
        addAndMakeVisible(masterSection);
        addAndMakeVisible(pitchEnvelopeSection);
        addAndMakeVisible(patternSection);

        fileManager.setOnStateChanged([this] { updateWindowTitle(); });
        processor.setOnDirtyChanged ([this] { updateWindowTitle(); });

        setWantsKeyboardFocus(true);

        setSize(900, 920 + kMenuBarHeight);
    }

    void MainComponent::paint(juce::Graphics& g)
    {
        g.fillAll(juce::Colour::fromRGB(22, 22, 22));
    }

    void MainComponent::resized()
    {
        auto fullBounds = getLocalBounds();
        menuBar.setBounds(fullBounds.removeFromTop(kMenuBarHeight));

        auto bounds = fullBounds.reduced(kOuterPadding);

        auto topRow = bounds.removeFromTop(kTopRowHeight);
        bounds.removeFromTop(kGap);
        auto midRow = bounds.removeFromTop(kMidRowHeight);
        bounds.removeFromTop(kGap);
        auto pitchRow = bounds.removeFromTop(kPitchRowHeight);
        bounds.removeFromTop(kGap);
        auto patternRow = bounds;

        const int topCellWidth = (topRow.getWidth() - 2 * kGap) / 3;
        oscillatorSection.setBounds(topRow.removeFromLeft(topCellWidth));
        topRow.removeFromLeft(kGap);
        ampEnvelopeSection.setBounds(topRow.removeFromLeft(topCellWidth));
        topRow.removeFromLeft(kGap);
        filterSection.setBounds(topRow);

        const int midCellWidth = (midRow.getWidth() - kGap) / 2;
        effectsSection.setBounds(midRow.removeFromLeft(midCellWidth));
        midRow.removeFromLeft(kGap);
        masterSection.setBounds(midRow);

        pitchEnvelopeSection.setBounds(pitchRow);
        patternSection.setBounds(patternRow);
    }

    void MainComponent::openProjectFile(const juce::File& file)
    {
        fileManager.openFile(file);
    }

    bool MainComponent::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::spaceKey)
        {
            processor.triggerAudition();
            return true;
        }

        const auto& mods = key.getModifiers();
        const bool cmd   = mods.isCommandDown();
        const bool shift = mods.isShiftDown();
        const int  code  = key.getKeyCode();

        if (cmd && shift && code == 'S')   { fileManager.saveAs(this); return true; }
        if (cmd && code == 'S')            { fileManager.save  (this); return true; }
        if (cmd && code == 'O')            { fileManager.open  (this); return true; }

        // Cmd+Z / Cmd+Shift+Z. Currently covers slider edits and
        // dice rolls (everything the APVTS owns). Pattern grid and
        // pitch-curve edits aren't routed through UndoManager yet —
        // that's a separate gap.
        if (cmd && shift && code == 'Z')   { processor.getUndoManager().redo(); return true; }
        if (cmd && code == 'Z')            { processor.getUndoManager().undo(); return true; }

        // Cmd+/ — About box. Slash is rarely used by JUCE controls
        // for anything else, so it bubbles up reliably.
        if (cmd && (code == '/' || key.getTextCharacter() == '/'))
        {
            showAboutDialog();
            return true;
        }

        return false;
    }

    void MainComponent::parentHierarchyChanged()
    {
        // The window doesn't exist while the constructor runs;
        // this fires once after setContentOwned attaches us to the
        // DocumentWindow, which is the right moment to sync the
        // initial title.
        updateWindowTitle();
    }

    void MainComponent::updateWindowTitle()
    {
        auto* dw = findParentComponentOfClass<juce::DocumentWindow>();
        if (dw == nullptr)
            return;

        const auto& file   = fileManager.getCurrentFile();
        const auto  name   = file == juce::File() ? juce::String("Untitled")
                                                     : file.getFileName();
        const auto  suffix = processor.isDirty() ? juce::String(" *")
                                                  : juce::String();
        dw->setName("b33p - " + name + suffix);
    }

    juce::StringArray MainComponent::getMenuBarNames()
    {
        return { "File", "Edit", "Help" };
    }

    juce::PopupMenu MainComponent::getMenuForIndex(int topLevelIndex,
                                                   const juce::String& /*menuName*/)
    {
        // Helper: build a menu item that also shows its keyboard
        // shortcut on the right-hand side. Item keeps the dispatch
        // ID used by menuItemSelected.
        auto withShortcut = [](int id, juce::String text,
                                juce::String shortcutText, bool enabled = true)
        {
            juce::PopupMenu::Item item;
            item.itemID                = id;
            item.text                  = std::move(text);
            item.shortcutKeyDescription = std::move(shortcutText);
            item.isEnabled             = enabled;
            return item;
        };

        juce::PopupMenu m;
        switch (topLevelIndex)
        {
            case 0: // File
                m.addItem(withShortcut(MenuId::FileOpen,   "Open...",    "Cmd+O"));
                m.addItem(withShortcut(MenuId::FileSave,   "Save",       "Cmd+S"));
                m.addItem(withShortcut(MenuId::FileSaveAs, "Save As...", "Cmd+Shift+S"));
                break;
            case 1: // Edit
                m.addItem(withShortcut(MenuId::EditUndo, "Undo", "Cmd+Z",
                                       processor.getUndoManager().canUndo()));
                m.addItem(withShortcut(MenuId::EditRedo, "Redo", "Cmd+Shift+Z",
                                       processor.getUndoManager().canRedo()));
                break;
            case 2: // Help
                m.addItem(withShortcut(MenuId::HelpAbout, "About b33p", "Cmd+/"));
                break;
            default:
                break;
        }
        return m;
    }

    void MainComponent::menuItemSelected(int menuItemId, int /*topLevelIndex*/)
    {
        switch (menuItemId)
        {
            case MenuId::FileOpen:   fileManager.open  (this);                  break;
            case MenuId::FileSave:   fileManager.save  (this);                  break;
            case MenuId::FileSaveAs: fileManager.saveAs(this);                  break;
            case MenuId::EditUndo:   processor.getUndoManager().undo();         break;
            case MenuId::EditRedo:   processor.getUndoManager().redo();         break;
            case MenuId::HelpAbout:  showAboutDialog();                         break;
            default:                                                            break;
        }
    }

    void MainComponent::showAboutDialog()
    {
        // String literals so any of version, author, or license can
        // be tweaked here without recompiling more than this file.
        const juce::String version  { B33P_VERSION_STRING };
        const juce::String author   { "ZQ SFX (themightyzq)" };
        const juce::String license  { "GPL-3.0-or-later" };
        const juce::String juceVer  { juce::SystemStats::getJUCEVersion() };

        const juce::String body =
            juce::String("b33p — small synthesized sounds.\n\n")
          + "Version "      + version + "\n"
          + "Author "       + author  + "\n"
          + "License "      + license + "\n\n"
          + "Built with " + juceVer + " and Catch2.\n\n"
          + "Source: github.com/themightyzq/b33p";

        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::InfoIcon)
                .withTitle("About b33p")
                .withMessage(body)
                .withButton("OK"),
            nullptr);
    }
}

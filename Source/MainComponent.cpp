#include "MainComponent.h"

namespace B33p
{
    namespace
    {
        constexpr int kOuterPadding   = 12;
        constexpr int kGap            = 8;
        constexpr int kTopRowHeight   = 260;
        constexpr int kMidRowHeight   = 180;
        constexpr int kModRowHeight   = 130;
        constexpr int kPitchRowHeight = 180;
        constexpr int kMenuBarHeight  = 24;

        // Menu item IDs. Kept in one enum so the dispatch in
        // menuItemSelected stays readable. IDs must be > 0 — JUCE
        // treats 0 as "nothing selected".
        //
        // Recent-files items live in their own ID range above all
        // the static items so we can dispatch them with a single
        // range check — see menuItemSelected.
        enum MenuId
        {
            FileNew = 1,
            FileOpen,
            FileSave,
            FileSaveAs,
            FileAudioSettings,
            FileClearRecent,
            EditUndo,
            EditRedo,
            EditCopy,
            EditPaste,
            EditSelectAll,
            EditDeselect,
            LaneCopyToAll,
            LaneResetVoice,
            LaneDiceAll,
            LaneGenerate,
            LaneClear,
            HelpAbout,

            FileOpenRecentBase = 1000,   // [1000, 1000 + N) reserved for MRU slots
            FileOpenRecentMax  = 1099,
        };
    }

    MainComponent::MainComponent(B33pProcessor& processorRef,
                                  juce::AudioDeviceManager& deviceManagerRef)
        : processor(processorRef),
          deviceManager(deviceManagerRef),
          fileManager(processorRef),
          oscillatorSection   (processor),
          ampEnvelopeSection  (processor),
          filterSection       (processor),
          effectsSection      (processor),
          modEffectsSection   (processor),
          masterSection       (processor),
          pitchEnvelopeSection(processor),
          patternSection      (processor)
    {
        addAndMakeVisible(menuBar);
        addAndMakeVisible(oscillatorSection);
        addAndMakeVisible(ampEnvelopeSection);
        addAndMakeVisible(filterSection);
        addAndMakeVisible(effectsSection);
        addAndMakeVisible(modEffectsSection);
        addAndMakeVisible(masterSection);
        addAndMakeVisible(pitchEnvelopeSection);
        addAndMakeVisible(patternSection);

        fileManager.setOnStateChanged([this] { updateWindowTitle(); });
        processor.setOnDirtyChanged ([this] { updateWindowTitle(); });

        // Resync the editors / pattern combos that don't auto-track
        // APVTS attachments after a bulk reload (Open or New).
        processor.setOnFullStateLoaded([this]
        {
            pitchEnvelopeSection.refreshFromState();
            patternSection      .refreshFromState();
            // Section attachments may now point at the wrong lane's
            // params; re-target everything against the loaded
            // selectedLane.
            const int lane = processor.getSelectedLane();
            oscillatorSection .retargetLane(lane);
            ampEnvelopeSection.retargetLane(lane);
            filterSection     .retargetLane(lane);
            effectsSection    .retargetLane(lane);
            modEffectsSection .retargetLane(lane);
            masterSection     .retargetLane(lane);
        });

        // Lane selection changes drive the per-lane editor sections to
        // rebuild their APVTS attachments and update their title
        // suffixes. The pattern grid repaints to refresh the lane tint.
        processor.setOnSelectedLaneChanged([this](int lane)
        {
            oscillatorSection .retargetLane(lane);
            ampEnvelopeSection.retargetLane(lane);
            filterSection     .retargetLane(lane);
            effectsSection    .retargetLane(lane);
            modEffectsSection .retargetLane(lane);
            masterSection     .retargetLane(lane);
            patternSection    .refreshFromState();   // repaint grid for tint
        });

        setWantsKeyboardFocus(true);

        setSize(900, 920 + kMenuBarHeight + kModRowHeight + kGap);
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
        auto modRow = bounds.removeFromTop(kModRowHeight);
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

        modEffectsSection.setBounds(modRow);

        pitchEnvelopeSection.setBounds(pitchRow);
        patternSection.setBounds(patternRow);
    }

    void MainComponent::openProjectFile(const juce::File& file)
    {
        fileManager.openFile(file);
    }

    void MainComponent::confirmDiscardThen(std::function<void()> proceed)
    {
        if (! processor.isDirty())
        {
            if (proceed) proceed();
            return;
        }

        // Button index in the result: 1 = Save, 2 = Discard, 0 = Cancel.
        // JUCE assigns 1..N for the order they're added; "Cancel" is
        // mapped to 0 by withButton when it is the last/escape choice.
        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::WarningIcon)
                .withTitle("Unsaved changes")
                .withMessage("This project has unsaved changes. Save before closing?")
                .withButton("Save")
                .withButton("Discard")
                .withButton("Cancel"),
            [this, next = std::move(proceed)](int result) mutable
            {
                if (result == 1) // Save
                {
                    fileManager.save(this,
                        [afterSave = std::move(next)](bool ok)
                        {
                            if (ok && afterSave) afterSave();
                        });
                }
                else if (result == 2) // Discard
                {
                    if (next) next();
                }
                // result == 0 (Cancel): drop the proceed callback.
            });
    }

    bool MainComponent::keyPressed(const juce::KeyPress& key)
    {
        // Spacebar = pattern play/stop (DAW convention).
        // Shift+Space = audition the voice (the old Space behaviour).
        if (key.getKeyCode() == juce::KeyPress::spaceKey)
        {
            if (key.getModifiers().isShiftDown())
                processor.triggerAudition();
            else if (processor.isPlaying())
                processor.stopPlayback();
            else
                processor.startPlayback();
            return true;
        }

        const auto& mods = key.getModifiers();
        const bool cmd   = mods.isCommandDown();
        const bool shift = mods.isShiftDown();
        const int  code  = key.getKeyCode();

        if (cmd && shift && code == 'S')   { fileManager.saveAs(this); return true; }
        if (cmd && code == 'S')            { fileManager.save  (this); return true; }
        if (cmd && code == 'O')
        {
            confirmDiscardThen([this] { fileManager.open(this); });
            return true;
        }
        if (cmd && code == 'N')
        {
            confirmDiscardThen([this] { fileManager.newProject(); });
            return true;
        }

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

    MainComponent::~MainComponent()
    {
        if (auto* host = keyListenerHost.getComponent())
            host->removeKeyListener(this);
    }

    void MainComponent::parentHierarchyChanged()
    {
        // The window doesn't exist while the constructor runs; this
        // fires once after setContentOwned attaches us to the
        // DocumentWindow. Sync the title, seed keyboard focus, and
        // hook a window-level KeyListener so Space + Shift+Space
        // work even when no component has focus (or when focus is
        // sitting on the menu bar / a button that doesn't bubble).
        updateWindowTitle();
        if (isShowing())
            grabKeyboardFocus();

        if (auto* tlw = getTopLevelComponent();
            tlw != nullptr && tlw != keyListenerHost.getComponent())
        {
            if (auto* prev = keyListenerHost.getComponent())
                prev->removeKeyListener(this);
            tlw->addKeyListener(this);
            keyListenerHost = tlw;
        }
    }

    bool MainComponent::keyPressed(const juce::KeyPress& key,
                                    juce::Component* originatingComponent)
    {
        // While a text input has focus, every keystroke (Space
        // included) belongs to the field. Don't intercept.
        if (dynamic_cast<juce::TextEditor*>(originatingComponent) != nullptr)
            return false;

        // Only handle the transport keys here — everything else
        // (Cmd+S, Cmd+Z, ...) goes through the Component-style
        // keyPressed via normal bubble-up.
        if (key.getKeyCode() == juce::KeyPress::spaceKey)
            return keyPressed(key);

        return false;
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
        return { "File", "Edit", "Lane", "Help" };
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
                m.addItem(withShortcut(MenuId::FileNew,    "New",        "Cmd+N"));
                m.addItem(withShortcut(MenuId::FileOpen,   "Open...",    "Cmd+O"));
                {
                    // Persistent MRU. Disabled when empty so the user
                    // gets a visual hint that the list will populate
                    // as they open / save projects.
                    const auto& recents = fileManager.getRecentFiles();
                    const int n = juce::jmin(recents.getNumFiles(),
                                             MenuId::FileOpenRecentMax - MenuId::FileOpenRecentBase);
                    juce::PopupMenu recentMenu;
                    for (int i = 0; i < n; ++i)
                        recentMenu.addItem(MenuId::FileOpenRecentBase + i,
                                           recents.getFile(i).getFileName());
                    if (n > 0)
                    {
                        recentMenu.addSeparator();
                        recentMenu.addItem(MenuId::FileClearRecent, "Clear Menu");
                    }
                    m.addSubMenu("Open Recent", recentMenu, n > 0);
                }
                m.addSeparator();
                m.addItem(withShortcut(MenuId::FileSave,   "Save",       "Cmd+S"));
                m.addItem(withShortcut(MenuId::FileSaveAs, "Save As...", "Cmd+Shift+S"));
                m.addSeparator();
                m.addItem(withShortcut(MenuId::FileAudioSettings, "Audio Settings...", ""));
                break;
            case 1: // Edit
                m.addItem(withShortcut(MenuId::EditUndo, "Undo", "Cmd+Z",
                                       processor.getUndoManager().canUndo()));
                m.addItem(withShortcut(MenuId::EditRedo, "Redo", "Cmd+Shift+Z",
                                       processor.getUndoManager().canRedo()));
                m.addSeparator();
                {
                    auto& grid = patternSection.getGrid();
                    m.addItem(withShortcut(MenuId::EditCopy,      "Copy Selected Events",     "Cmd+C", grid.hasSelection()));
                    m.addItem(withShortcut(MenuId::EditPaste,     "Paste at Playhead",        "Cmd+V", grid.hasClipboardData()));
                    m.addItem(withShortcut(MenuId::EditSelectAll, "Select All Events",        "Cmd+A", grid.hasAnyEvents()));
                    m.addItem(withShortcut(MenuId::EditDeselect,  "Deselect",                 "Esc",   grid.hasSelection()));
                }
                break;
            case 2: // Lane (the currently-selected one)
            {
                const int lane = processor.getSelectedLane();
                const juce::String tag = "Lane " + juce::String(lane + 1);
                m.addItem(MenuId::LaneCopyToAll,  "Copy " + tag + " voice to all lanes");
                m.addItem(MenuId::LaneResetVoice, "Reset " + tag + " voice to defaults");
                m.addSeparator();
                m.addItem(MenuId::LaneGenerate,   "Generate Random Pattern in " + tag);
                m.addItem(MenuId::LaneClear,      "Clear All Events in " + tag);
                m.addSeparator();
                m.addItem(MenuId::LaneDiceAll,    "Randomize All Lanes (every unlocked param)");
                break;
            }
            case 3: // Help
                m.addItem(withShortcut(MenuId::HelpAbout, "About b33p", "Cmd+/"));
                break;
            default:
                break;
        }
        return m;
    }

    void MainComponent::menuItemSelected(int menuItemId, int /*topLevelIndex*/)
    {
        // MRU items live in a contiguous range so a single check
        // routes them all through the same discard-confirmation
        // path that File ▸ Open uses.
        if (menuItemId >= MenuId::FileOpenRecentBase
            && menuItemId <= MenuId::FileOpenRecentMax)
        {
            const int index = menuItemId - MenuId::FileOpenRecentBase;
            confirmDiscardThen([this, index] { fileManager.openRecentFile(index); });
            return;
        }

        switch (menuItemId)
        {
            case MenuId::FileNew:
                confirmDiscardThen([this] { fileManager.newProject(); });
                break;
            case MenuId::FileOpen:
                confirmDiscardThen([this] { fileManager.open(this); });
                break;
            case MenuId::FileSave:           fileManager.save  (this);          break;
            case MenuId::FileSaveAs:         fileManager.saveAs(this);          break;
            case MenuId::FileAudioSettings:  showAudioSettings();               break;
            case MenuId::FileClearRecent:    fileManager.clearRecentFiles();    break;
            case MenuId::EditUndo:        processor.getUndoManager().undo(); break;
            case MenuId::EditCopy:        patternSection.getGrid().copySelectedToClipboard();      break;
            case MenuId::EditPaste:       patternSection.getGrid().pasteFromClipboardAtPlayhead(); break;
            case MenuId::EditSelectAll:   patternSection.getGrid().selectAll();                    break;
            case MenuId::EditDeselect:    patternSection.getGrid().clearSelection();               break;
            case MenuId::EditRedo:   processor.getUndoManager().redo();         break;
            case MenuId::LaneCopyToAll:
                processor.copyLaneSettingsToAll(processor.getSelectedLane());   break;
            case MenuId::LaneResetVoice:
                processor.resetLaneVoice(processor.getSelectedLane());          break;
            case MenuId::LaneDiceAll:
            {
                juce::Random rng;
                processor.getRandomizer().rollAllUnlocked(rng);
                break;
            }
            case MenuId::LaneGenerate:
                patternSection.getGrid().generateRandomPatternInLane(
                    processor.getSelectedLane());
                break;
            case MenuId::LaneClear:
                patternSection.getGrid().clearAllEventsInLane(
                    processor.getSelectedLane());
                break;
            case MenuId::HelpAbout:  showAboutDialog();                         break;
            default:                                                            break;
        }
    }

    void MainComponent::showAudioSettings()
    {
        if (audioSettingsWindow == nullptr)
            audioSettingsWindow = std::make_unique<AudioSettingsWindow>(deviceManager);
        audioSettingsWindow->setVisible(true);
        audioSettingsWindow->toFront(true);
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
            juce::String("b33p: small synthesized sounds.\n\n")
          + "Version "      + version + "\n"
          + "Author "       + author  + "\n"
          + "License "      + license + "\n\n"
          + "Built with " + juceVer + " and Catch2.\n"
          + "Source: github.com/themightyzq/b33p\n\n"
          + "------ How it's organized ------\n"
          + "Each of the 4 pattern lanes has its own voice. The voice\n"
          + "editor sections (Oscillator, Amp Envelope, Filter, Effects,\n"
          + "Master) say \"(Lane N)\" because they edit whichever lane is\n"
          + "currently selected. Click a lane (or any of its events) in\n"
          + "the pattern grid to switch which lane the editor targets.\n"
          + "The Pitch Envelope is shared across all four lanes.\n\n"
          + "------ Pattern editing ------\n"
          + "  Drag in empty grid: draw an event of any length\n"
          + "  Double-click empty grid: drop an event at default size\n"
          + "  Drag a clip: move it; drag vertically to switch lanes\n"
          + "  Drag a clip's left/right edge: resize\n"
          + "  Drag a clip's top edge: set velocity (clip height)\n"
          + "  Right-click a clip: Delete / Duplicate\n"
          + "  Right-click empty lane: Generate / Clear\n"
          + "  Click in the ruler row: park the playhead\n\n"
          + "------ Keys ------\n"
          + "  Space               play / stop\n"
          + "  Shift+Space         audition the selected lane\n"
          + "  Cmd+A / C / V       select all / copy / paste at playhead\n"
          + "  Esc                 deselect\n"
          + "  Arrow / Shift+Arrow nudge by 1 / 10 grid steps\n"
          + "  Cmd+Z / Shift+Z     undo / redo";

        // "Close" is added first so it's the default/Enter-key
        // button — clicking the GitHub button should be deliberate,
        // not what hammering Return does.
        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::InfoIcon)
                .withTitle("About b33p")
                .withMessage(body)
                .withButton("Close")
                .withButton("Open GitHub Page"),
            [](int result)
            {
                if (result == 2)
                    juce::URL("https://github.com/themightyzq/b33p").launchInDefaultBrowser();
            });
    }
}

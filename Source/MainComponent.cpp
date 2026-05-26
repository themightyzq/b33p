#include "MainComponent.h"

#include "State/ProjectState.h"
#include "State/UndoableActions.h"

namespace B33p
{
    namespace
    {
        constexpr int kOuterPadding   = 12;
        constexpr int kGap            = 8;
        constexpr int kTopRowHeight        = 260;   // Oscillator | Amp Env | Filter
        constexpr int kMidRowHeight        = 180;   // Effects | Master | Mod FX
        constexpr int kModulationRowHeight = 220;   // Modulation | Pitch Env
        constexpr int kMenuBarHeight  = 24;
        // Initial height handed to the Pattern grid; it grows to fill any
        // extra height when the window is taller than the default.
        constexpr int kInitialPatternHeight = 252;

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
            FileSavePreset,
            FileBrowsePresets,
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
            HelpKeyboardShortcuts,
            HelpAudioSettings,

            FileOpenRecentBase = 1000,   // [1000, 1000 + N) reserved for MRU slots
            FileOpenRecentMax  = 1099,
        };
    }

    MainComponent::MainComponent(B33pProcessor& processorRef)
        : processor(processorRef),
          fileManager(processorRef),
          presetManager(processorRef),
          // Seed factory presets the first time we see an empty
          // presets directory. Existing files (whether shipped or
          // user-modified) are never overwritten — see
          // PresetManager::seedFactoryPresetsIfMissing for the
          // policy. The seed call sits here rather than in
          // PresetManager's constructor so a unit test wanting an
          // empty directory can construct a PresetManager without
          // side effects.
          // (Constructor body below runs the seed.)
          oscillatorSection   (processor),
          ampEnvelopeSection  (processor),
          filterSection       (processor),
          effectsSection      (processor),
          modEffectsSection   (processor),
          modulationSection   (processor),
          masterSection       (processor),
          pitchEnvelopeSection(processor),
          patternSection      (processor)
    {
        // Seed factory presets so a fresh install has discoverable
        // starting points. seedFactoryPresetsIfMissing skips any
        // file the user has tweaked, so this is safe to call on
        // every launch.
        presetManager.seedFactoryPresetsIfMissing();

        addAndMakeVisible(menuBar);
        addAndMakeVisible(oscillatorSection);
        addAndMakeVisible(ampEnvelopeSection);
        addAndMakeVisible(filterSection);
        addAndMakeVisible(effectsSection);
        addAndMakeVisible(modEffectsSection);
        addAndMakeVisible(modulationSection);
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
            modulationSection .retargetLane(lane);
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
            modulationSection .retargetLane(lane);
            masterSection     .retargetLane(lane);
            patternSection    .refreshFromState();   // repaint grid for tint
        });

        setWantsKeyboardFocus(true);

        // 1500 wide accommodates the Pattern toolbar's full set of controls
        // (Play / Loop / Follow / time readout / Length / Grid / BPM / Time-sig
        // on the left, Randomize All / Scope / Export on the right — see
        // PatternSection::resized). At the old 900 width, BPM + Time-sig +
        // the Length label were squeezed into zero-width slivers.
        //
        // Height = menu + padding + the three voice-editor rows + an initial
        // Pattern grid. The voice editor is column-packed (Mod FX shares the
        // mid row, Pitch Env shares the modulation row) so the whole window
        // fits a 1080p display instead of overflowing it. The standalone
        // window additionally clamps to the screen on launch — see
        // StandaloneApp.cpp.
        setSize(1500,
                kMenuBarHeight + 2 * kOuterPadding
              + kTopRowHeight + kGap
              + kMidRowHeight + kGap
              + kModulationRowHeight + kGap
              + kInitialPatternHeight);
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

        // Three voice-editor rows stacked above the Pattern grid. Mod FX and
        // Pitch Env — both wide-but-short — share rows with their neighbours
        // rather than each claiming a full-width row of their own, which is
        // what kept the old six-row stack taller than a 1080p screen.
        auto topRow = bounds.removeFromTop(kTopRowHeight);
        bounds.removeFromTop(kGap);
        auto midRow = bounds.removeFromTop(kMidRowHeight);
        bounds.removeFromTop(kGap);
        auto modulationRow = bounds.removeFromTop(kModulationRowHeight);
        bounds.removeFromTop(kGap);
        auto patternRow = bounds;

        // Top row: Oscillator | Amp Env | Filter (three equal columns).
        const int topCellWidth = (topRow.getWidth() - 2 * kGap) / 3;
        oscillatorSection.setBounds(topRow.removeFromLeft(topCellWidth));
        topRow.removeFromLeft(kGap);
        ampEnvelopeSection.setBounds(topRow.removeFromLeft(topCellWidth));
        topRow.removeFromLeft(kGap);
        filterSection.setBounds(topRow);

        // Mid row: Effects | Master | Mod FX (three equal columns).
        const int midCellWidth = (midRow.getWidth() - 2 * kGap) / 3;
        effectsSection.setBounds(midRow.removeFromLeft(midCellWidth));
        midRow.removeFromLeft(kGap);
        masterSection.setBounds(midRow.removeFromLeft(midCellWidth));
        midRow.removeFromLeft(kGap);
        modEffectsSection.setBounds(midRow);

        // Modulation row: Modulation | Pitch Env (two equal columns).
        const int modCellWidth = (modulationRow.getWidth() - kGap) / 2;
        modulationSection.setBounds(modulationRow.removeFromLeft(modCellWidth));
        modulationRow.removeFromLeft(kGap);
        pitchEnvelopeSection.setBounds(modulationRow);

        patternSection.setBounds(patternRow);
    }

    void MainComponent::openProjectFile(const juce::File& file)
    {
        fileManager.openFile(file);
    }

    void MainComponent::confirmActionThen(const juce::String& message,
                                           std::function<void()> proceed)
    {
        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::WarningIcon)
                .withTitle("Confirm")
                .withMessage(message)
                .withButton("Cancel")     // index 1 — Escape default
                .withButton("Proceed"),   // index 2
            [next = std::move(proceed)](int result)
            {
                if (result == 2 && next) next();
            });
    }

    void MainComponent::confirmDiscardThen(std::function<void()> proceed)
    {
        if (! processor.isDirty())
        {
            if (proceed) proceed();
            return;
        }

        // Pull the project filename into the question — that's the
        // anchor the user is mentally tracking, not the abstract
        // "this project." Falls back to "Untitled" for never-saved
        // projects.
        const auto& currentFile = fileManager.getCurrentFile();
        const auto fileName     = currentFile == juce::File()
                                    ? juce::String("Untitled")
                                    : currentFile.getFileName();

        // Button index in the result: 1 = Save, 2 = Discard, 0 = Cancel.
        // JUCE assigns 1..N for the order they're added; "Cancel" is
        // mapped to 0 by withButton when it is the last/escape choice.
        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::WarningIcon)
                .withTitle("Unsaved changes")
                .withMessage("Save changes to " + fileName + " before closing?")
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
        // "Lane" used to be a top-level menu — demoted to Edit ▸ Lane
        // because everything inside it scoped to "the currently selected
        // lane," which is context-menu territory rather than a peer of
        // File/Edit. The same actions also live on the lane right-click
        // menu in PatternGrid.
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
                m.addItem(withShortcut(MenuId::FileNew,    "New",        "Cmd+N"));
                m.addItem(withShortcut(MenuId::FileOpen,   "Open...",    "Cmd+O"));
                {
                    // Persistent MRU. When empty, the submenu stays
                    // enabled and contains a single disabled "No
                    // recent files" placeholder — a greyed-out parent
                    // with no children reads as "feature broken,"
                    // whereas an enabled submenu with a disabled
                    // placeholder reads as "feature inactive."
                    const auto& recents = fileManager.getRecentFiles();
                    const int n = juce::jmin(recents.getNumFiles(),
                                             MenuId::FileOpenRecentMax - MenuId::FileOpenRecentBase);
                    juce::PopupMenu recentMenu;
                    if (n == 0)
                    {
                        juce::PopupMenu::Item placeholder;
                        placeholder.text      = "No recent files";
                        placeholder.isEnabled = false;
                        recentMenu.addItem(placeholder);
                    }
                    else
                    {
                        for (int i = 0; i < n; ++i)
                            recentMenu.addItem(MenuId::FileOpenRecentBase + i,
                                               recents.getFile(i).getFileName());
                        recentMenu.addSeparator();
                        recentMenu.addItem(MenuId::FileClearRecent, "Clear Menu");
                    }
                    m.addSubMenu("Open Recent", recentMenu);
                }
                m.addSeparator();
                m.addItem(withShortcut(MenuId::FileSave,   "Save",       "Cmd+S"));
                m.addItem(withShortcut(MenuId::FileSaveAs, "Save As...", "Cmd+Shift+S"));
                m.addSeparator();
                m.addItem(withShortcut(MenuId::FileSavePreset,    "Save Preset...",   ""));
                m.addItem(withShortcut(MenuId::FileBrowsePresets, "Browse Presets...", ""));
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
                m.addSeparator();
                {
                    // Lane submenu (formerly a top-level menu). All items
                    // scope to the currently-selected lane; the label
                    // regenerates with the lane tag so it's clear which
                    // one each action will affect.
                    const int lane = processor.getSelectedLane();
                    const juce::String tag = "Lane " + juce::String(lane + 1);
                    juce::PopupMenu laneMenu;
                    laneMenu.addItem(MenuId::LaneCopyToAll,  "Copy " + tag + " voice to all lanes");
                    laneMenu.addItem(MenuId::LaneResetVoice, "Reset " + tag + " voice to defaults");
                    laneMenu.addSeparator();
                    laneMenu.addItem(MenuId::LaneGenerate,   "Generate Random Pattern in " + tag);
                    laneMenu.addItem(MenuId::LaneClear,      "Clear All Events in " + tag);
                    laneMenu.addSeparator();
                    laneMenu.addItem(MenuId::LaneDiceAll,    "Randomize All Lanes (every unlocked param)");
                    m.addSubMenu("Lane", laneMenu);
                }
                break;
            case 2: // Help
                m.addItem(withShortcut(MenuId::HelpKeyboardShortcuts, "Keyboard Shortcuts...", ""));
                m.addItem(withShortcut(MenuId::HelpAudioSettings,    "Audio Settings...",     ""));
                m.addSeparator();
                m.addItem(withShortcut(MenuId::HelpAbout, "About b33p", ""));
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
            case MenuId::FileSavePreset:     promptSavePreset();                break;
            case MenuId::FileBrowsePresets:  showPresetBrowser();               break;
            case MenuId::FileClearRecent:    fileManager.clearRecentFiles();    break;
            case MenuId::EditUndo:        processor.getUndoManager().undo(); break;
            case MenuId::EditCopy:        patternSection.getGrid().copySelectedToClipboard();      break;
            case MenuId::EditPaste:       patternSection.getGrid().pasteFromClipboardAtPlayhead(); break;
            case MenuId::EditSelectAll:   patternSection.getGrid().selectAll();                    break;
            case MenuId::EditDeselect:    patternSection.getGrid().clearSelection();               break;
            case MenuId::EditRedo:   processor.getUndoManager().redo();         break;
            case MenuId::LaneCopyToAll:
            {
                const int lane = processor.getSelectedLane();
                confirmActionThen(
                    "Copy Lane " + juce::String(lane + 1)
                        + "'s voice to every other lane?\n\n"
                          "This overwrites the other three lanes' voice parameters.",
                    [this, lane] { processor.copyLaneSettingsToAll(lane); });
                break;
            }
            case MenuId::LaneResetVoice:
            {
                const int lane = processor.getSelectedLane();
                confirmActionThen(
                    "Reset Lane " + juce::String(lane + 1)
                        + "'s voice to defaults?",
                    [this, lane] { processor.resetLaneVoice(lane); });
                break;
            }
            case MenuId::LaneDiceAll:
                confirmActionThen(
                    "Randomize every unlocked parameter across all four lanes?",
                    [this]
                    {
                        juce::Random rng;
                        processor.getRandomizer().rollAllUnlocked(rng);
                    });
                break;
            case MenuId::LaneGenerate:
            {
                const int lane = processor.getSelectedLane();
                confirmActionThen(
                    "Generate a fresh random pattern in Lane "
                        + juce::String(lane + 1)
                        + "?\n\nThis replaces any existing events in that lane.",
                    [this, lane]
                    {
                        patternSection.getGrid().generateRandomPatternInLane(lane);
                    });
                break;
            }
            case MenuId::LaneClear:
            {
                const int lane = processor.getSelectedLane();
                confirmActionThen(
                    "Clear all events in Lane " + juce::String(lane + 1) + "?",
                    [this, lane]
                    {
                        patternSection.getGrid().clearAllEventsInLane(lane);
                    });
                break;
            }
            case MenuId::HelpAbout:              showAboutDialog();              break;
            case MenuId::HelpKeyboardShortcuts:  showKeyboardShortcutsDialog();  break;
            case MenuId::HelpAudioSettings:      showAudioSettingsHelpDialog();  break;
            default:                                                            break;
        }
    }

    void MainComponent::showPresetBrowser()
    {
        // Lazy-construct so the dialog only allocates when first
        // requested. Once built, we keep it around and toggle
        // visibility — preserves window position between visits.
        if (presetBrowserWindow == nullptr)
        {
            presetBrowserWindow = std::make_unique<PresetBrowserDialogWindow>(
                presetManager,
                // Load: route through the same dirty-prompt the
                // File ▸ Open path uses so the user can't lose
                // unsaved changes by clicking a preset.
                [this](const juce::File& presetFile)
                {
                    confirmDiscardThen([this, presetFile]
                    {
                        // P6: wrap the load in one undoable transaction so
                        // Cmd+Z (or the in-plugin Undo button) restores the
                        // previous patch instead of leaving it lost. Parse the
                        // preset, snapshot the full pre-load state, and swap
                        // via a LoadProjectStateAction. The preset is NOT
                        // promoted to the project save target — presets and
                        // projects are separate concepts.
                        if (! presetFile.existsAsFile())
                            return;
                        auto after = ProjectState::fromXmlString(presetFile.loadFileAsString());
                        if (! after.isValid())
                            return;   // same silent no-op the old loadPreset path had

                        auto before = ProjectState::save(processor);
                        processor.getUndoManager().beginNewTransaction("Load preset");
                        processor.getUndoManager().perform(
                            new LoadProjectStateAction(processor, this,
                                                       std::move(before), std::move(after)));
                    });
                },
                [this](const juce::File& presetFile)
                {
                    if (presetManager.deletePreset(presetFile))
                        if (presetBrowserWindow != nullptr)
                            presetBrowserWindow->refresh();
                },
                [this] { presetBrowserWindow.reset(); });
        }
        presetBrowserWindow->setVisible(true);
        presetBrowserWindow->toFront(true);
    }

    void MainComponent::promptSavePreset()
    {
        // AlertWindow with a single text field is the lightest
        // standard JUCE pattern for "name this thing". Modal so the
        // user can't keep clicking File ▸ Save Preset before the
        // first save resolves.
        auto* aw = new juce::AlertWindow("Save Preset",
                                          "Enter a name for the new preset:",
                                          juce::MessageBoxIconType::QuestionIcon);
        aw->addTextEditor("name", "", "Preset name");
        aw->addButton("Save",   1, juce::KeyPress(juce::KeyPress::returnKey));
        aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        savePresetWindow.reset(aw);
        aw->enterModalState(true,
            juce::ModalCallbackFunction::create(
                [this, aw](int result)
                {
                    const juce::String name = aw->getTextEditorContents("name");
                    savePresetWindow.reset();   // also deletes aw

                    if (result != 1 || name.trim().isEmpty())
                        return;

                    // P26: a name mapping to an existing preset gets a
                    // confirm before we clobber it; otherwise save straight
                    // away. Both paths run savePresetNamed below.
                    if (presetManager.getPresetFile(name).existsAsFile())
                    {
                        confirmActionThen(
                            "A preset with that name already exists. Overwrite it?",
                            [this, name] { savePresetNamed(name); });
                        return;
                    }

                    savePresetNamed(name);
                }),
            false);   // we own the AlertWindow, don't let the modal
                       // helper delete it
    }

    void MainComponent::savePresetNamed(const juce::String& name)
    {
        const auto saved = presetManager.savePreset(name);
        if (saved == juce::File())
        {
            // Actionable copy: name the two likely causes
            // and the on-disk path so the user can fix
            // permissions or rename, rather than guessing.
            const juce::String message =
                "Could not save preset \"" + name + "\".\n\n"
                "The name may contain invalid characters "
                "(avoid / \\ : * ? \" |), or the presets folder "
                "is not writable:\n"
              + presetManager.getPresetsDirectory().getFullPathName();

            juce::AlertWindow::showAsync(
                juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::WarningIcon)
                    .withTitle("Save Preset failed")
                    .withMessage(message)
                    .withButton("OK"),
                nullptr);
            return;
        }

        // If the browser is open, repopulate so the new
        // preset shows up.
        if (presetBrowserWindow != nullptr)
            presetBrowserWindow->refresh();
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
          + "Mod FX, Modulation, Master) say \"(Lane N)\" because they\n"
          + "edit whichever lane is currently selected. Click a lane\n"
          + "(or any of its events) in the pattern grid to switch which\n"
          + "lane the editor targets. The Pitch Envelope is shared\n"
          + "across all four lanes.\n\n"
          + "------ Pattern editing ------\n"
          + "  Drag in empty grid: draw an event of any length\n"
          + "  Double-click empty grid: drop an event at default size\n"
          + "  Drag a clip: move it; drag vertically to switch lanes\n"
          + "  Drag a clip's left/right edge: resize\n"
          + "  Drag a clip's top edge: set velocity (clip height)\n"
          + "  Right-click a clip: Delete / Duplicate / Edit overrides...\n"
          + "  Right-click empty lane: Generate / Clear\n"
          + "  Click in the ruler row: park the playhead\n\n"
          + "------ Playing ------\n"
          + "  Connect a MIDI keyboard — every input device routes to\n"
          + "  the selected lane (note 60 = no transposition). Up to 8\n"
          + "  notes ring at once.\n"
          + "  Loop toggle plays the pattern in a loop.\n"
          + "  Follow toggle (plugin mode) mirrors host transport.\n\n"
          + "------ Presets ------\n"
          + "  File > Save Preset / Browse Presets manage the per-user\n"
          + "  presets directory. Four factory patches ship with the\n"
          + "  app and seed the directory on first launch.\n\n"
          + "------ Keys ------\n"
          + "  See Help > Keyboard Shortcuts... for the full reference.";

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

    void MainComponent::showAudioSettingsHelpDialog()
    {
        // Static informational dialog rather than programmatic launch of
        // the wrapper's settings dialog — that would need access to
        // juce::StandalonePluginHolder::getInstance(), which requires
        // standalone-specific compilation that the shared editor TU
        // doesn't have. A custom standalone wrapper (the same plumbing
        // listed under TODO's "Deferred regressions") would let us open
        // it directly; until then, a sign pointing at the door is
        // strictly better than no door at all.
        const juce::String body =
            juce::String("Standalone b33p:\n")
          + "Click the small Options gear icon at the top-left of the window. "
            "It opens JUCE's audio device settings (output device, sample rate, "
            "buffer size).\n\n"
          + "Plugin mode (VST3 / AU):\n"
          + "Your DAW manages the audio device. If b33p is silent, check the "
            "track's I/O routing and the master output bus in the host.";

        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::InfoIcon)
                .withTitle("Audio Settings")
                .withMessage(body)
                .withButton("OK"),
            nullptr);
    }

    void MainComponent::showKeyboardShortcutsDialog()
    {
        // Mirrors the README's shortcut table grouping (Transport,
        // File / Edit, Pattern editing) so users see the same shape
        // whether they're reading docs or the in-app reference.
        const juce::String body =
            juce::String("Transport\n")
          + "  Space               play / stop\n"
          + "  Shift+Space         audition the selected lane's voice\n\n"
          + "File / Edit\n"
          + "  Cmd+N               new project\n"
          + "  Cmd+O               open project\n"
          + "  Cmd+S               save\n"
          + "  Cmd+Shift+S         save as...\n"
          + "  Cmd+Z / Cmd+Shift+Z undo / redo\n\n"
          + "Pattern editing (when the pattern grid has focus)\n"
          + "  Cmd+A               select every event\n"
          + "  Cmd+C               copy the selected events\n"
          + "  Cmd+V               paste the clipboard at the playhead\n"
          + "  Delete / Backspace  delete the selected events\n"
          + "  Arrow keys          nudge selected events by one grid step\n"
          + "  Shift+Arrow keys    nudge by ten grid steps\n"
          + "  Esc                 deselect everything\n\n"
          + "On Windows / Linux, swap Cmd for Ctrl.";

        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::InfoIcon)
                .withTitle("Keyboard Shortcuts")
                .withMessage(body)
                .withButton("Close"),
            nullptr);
    }
}

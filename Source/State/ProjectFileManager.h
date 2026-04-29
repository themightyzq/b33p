#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <functional>
#include <memory>

namespace B33p
{
    class B33pProcessor;

    // UI-side wrapper around ProjectState's disk I/O. Owns the
    // currently-open file path, runs juce::FileChooser dialogs for
    // Save As / Open, calls ProjectState::writeToFile /
    // readFromFile, and surfaces success/failure as juce::AlertWindow
    // pop-ups.
    //
    // onStateChanged fires whenever the current file changes (Save
    // As to a new path, Open succeeds). The MainComponent uses this
    // to keep the window title in sync with the open project.
    //
    // All file dialogs are async — the manager keeps the FileChooser
    // alive as a member so the OS sheet outlives the call site.
    class ProjectFileManager
    {
    public:
        explicit ProjectFileManager(B33pProcessor& processor);

        using OnStateChanged = std::function<void()>;
        void setOnStateChanged(OnStateChanged callback);

        const juce::File& getCurrentFile() const noexcept { return currentFile; }

        // Cmd+S: writes to the open file if there is one, otherwise
        // forwards to saveAs(). parent is the component to centre
        // the file chooser sheet over. The optional onComplete
        // callback fires with the outcome — used by the close-while-
        // dirty prompt to gate the quit on a successful save.
        using OnSaveComplete = std::function<void(bool success)>;
        void save  (juce::Component* parent, OnSaveComplete onComplete = {});
        void saveAs(juce::Component* parent, OnSaveComplete onComplete = {});
        void open  (juce::Component* parent);

        // Wipes the project to a freshly-launched state and clears
        // the current file so the next Save acts like a Save As.
        // Caller is responsible for any "discard unsaved changes?"
        // confirmation — newProject itself is non-destructive about
        // dirty state and will happily blow it away.
        void newProject();

        // Direct-open path for OS-driven file open events
        // (double-click on a .beep file in Finder, drag onto
        // the dock icon, etc.). Skips the chooser dialog and
        // loads the supplied file straight away. Same alert /
        // state-changed behaviour as the dialog-driven open path.
        void openFile(const juce::File& file);

        // Persistent MRU list of opened / saved files. Files are
        // pushed onto the list whenever an open or save succeeds,
        // and the list is restored from a properties file at
        // construction time. The MainComponent reads it to build
        // the File ▸ Open Recent submenu.
        const juce::RecentlyOpenedFilesList& getRecentFiles() const noexcept { return recentFiles; }

        // Loads the recent-list entry at `index` via openFile().
        // Out-of-range indices and missing files are no-ops; missing
        // files are also pruned from the list so the menu self-cleans.
        // Caller is responsible for any "discard unsaved changes?"
        // confirmation — same convention as openFile().
        void openRecentFile(int index);

        // Empties the MRU list and persists the change.
        void clearRecentFiles();

    private:
        void writeAndReport(const juce::File& destination,
                            OnSaveComplete onComplete = {});
        void readAndReport (const juce::File& source);

        // Push `file` onto the MRU list and persist. Called from
        // both the save and open code paths so a project that
        // round-trips Save As → Open shows up exactly once.
        void rememberRecentFile(const juce::File& file);
        void persistRecentFiles();

        B33pProcessor&                     processor;
        juce::File                         currentFile;
        OnStateChanged                     onStateChangedCallback;
        std::unique_ptr<juce::FileChooser> fileChooser;

        // Owns the on-disk MRU. Properties file lives under the
        // platform's per-user app-data directory; the list is
        // capped at kMaxRecentFiles entries.
        std::unique_ptr<juce::PropertiesFile> properties;
        juce::RecentlyOpenedFilesList         recentFiles;
    };
}

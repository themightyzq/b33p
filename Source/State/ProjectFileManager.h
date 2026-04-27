#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

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
        // the file chooser sheet over.
        void save  (juce::Component* parent);
        void saveAs(juce::Component* parent);
        void open  (juce::Component* parent);

        // Direct-open path for OS-driven file open events
        // (double-click on a .beep file in Finder, drag onto
        // the dock icon, etc.). Skips the chooser dialog and
        // loads the supplied file straight away. Same alert /
        // state-changed behaviour as the dialog-driven open path.
        void openFile(const juce::File& file);

    private:
        void writeAndReport(const juce::File& destination);
        void readAndReport (const juce::File& source);

        B33pProcessor&                     processor;
        juce::File                         currentFile;
        OnStateChanged                     onStateChangedCallback;
        std::unique_ptr<juce::FileChooser> fileChooser;
    };
}

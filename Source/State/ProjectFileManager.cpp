#include "ProjectFileManager.h"

#include "B33pProcessor.h"
#include "ProjectState.h"

namespace B33p
{
    namespace
    {
        constexpr const char* kFilePattern    = "*.beep";
        constexpr const char* kRecentFilesKey = "recentFiles";
        constexpr int         kMaxRecentFiles = 10;

        juce::File defaultStartLocation(const juce::File& currentFile)
        {
            if (currentFile != juce::File())
                return currentFile.getParentDirectory();
            return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
        }

        void showError(const juce::String& title, const juce::String& message)
        {
            juce::AlertWindow::showAsync(
                juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::WarningIcon)
                    .withTitle(title)
                    .withMessage(message)
                    .withButton("OK"),
                nullptr);
        }

        std::unique_ptr<juce::PropertiesFile> makePropertiesFile()
        {
            juce::PropertiesFile::Options options;
            options.applicationName     = "b33p";
            options.filenameSuffix      = ".settings";
            options.folderName          = "b33p";
            options.osxLibrarySubFolder = "Application Support";
            return std::make_unique<juce::PropertiesFile>(options);
        }
    }

    ProjectFileManager::ProjectFileManager(B33pProcessor& processorRef)
        : processor(processorRef),
          properties(makePropertiesFile())
    {
        recentFiles.restoreFromString(properties->getValue(kRecentFilesKey));
        recentFiles.setMaxNumberOfItems(kMaxRecentFiles);
    }

    void ProjectFileManager::setOnStateChanged(OnStateChanged callback)
    {
        onStateChangedCallback = std::move(callback);
    }

    void ProjectFileManager::save(juce::Component* parent, OnSaveComplete onComplete)
    {
        if (currentFile == juce::File())
        {
            saveAs(parent, std::move(onComplete));
            return;
        }
        writeAndReport(currentFile, std::move(onComplete));
    }

    void ProjectFileManager::saveAs(juce::Component* parent, OnSaveComplete onComplete)
    {
        const auto initial = defaultStartLocation(currentFile)
                                 .getChildFile(currentFile != juce::File()
                                                   ? currentFile.getFileName()
                                                   : juce::String("Untitled.beep"));

        fileChooser = std::make_unique<juce::FileChooser>(
            "Save b33p project...", initial, kFilePattern);

        const int flags = juce::FileBrowserComponent::saveMode
                        | juce::FileBrowserComponent::canSelectFiles
                        | juce::FileBrowserComponent::warnAboutOverwriting;

        fileChooser->launchAsync(flags,
            [this, parent, capturedOnComplete = std::move(onComplete)](const juce::FileChooser& fc) mutable
        {
            juce::ignoreUnused(parent);
            const auto chosen = fc.getResult();
            if (chosen == juce::File())
            {
                if (capturedOnComplete) capturedOnComplete(false);
                return;
            }
            // Force the .beep extension if the user dropped or
            // edited it away — keeps the file-type association
            // and the Open dialog filter both predictable.
            writeAndReport(chosen.withFileExtension(".beep"), std::move(capturedOnComplete));
        });
    }

    void ProjectFileManager::open(juce::Component* parent)
    {
        const auto initial = defaultStartLocation(currentFile);

        fileChooser = std::make_unique<juce::FileChooser>(
            "Open b33p project...", initial, kFilePattern);

        const int flags = juce::FileBrowserComponent::openMode
                        | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(flags, [this, parent](const juce::FileChooser& fc)
        {
            juce::ignoreUnused(parent);
            const auto chosen = fc.getResult();
            if (chosen == juce::File())
                return;
            readAndReport(chosen);
        });
    }

    void ProjectFileManager::openFile(const juce::File& file)
    {
        if (file != juce::File())
            readAndReport(file);
    }

    void ProjectFileManager::newProject()
    {
        processor.resetToDefaults();
        if (currentFile != juce::File())
        {
            currentFile = juce::File();
            if (onStateChangedCallback)
                onStateChangedCallback();
        }
    }

    void ProjectFileManager::openRecentFile(int index)
    {
        if (index < 0 || index >= recentFiles.getNumFiles())
            return;

        const auto file = recentFiles.getFile(index);
        if (! file.existsAsFile())
        {
            // Self-clean: a stale entry pointing at a moved/deleted
            // file gets pruned the moment the user tries to open it,
            // so the menu doesn't accumulate dead links forever.
            recentFiles.removeFile(file);
            persistRecentFiles();
            showError("Open failed",
                      "Could not find:\n" + file.getFullPathName()
                      + "\n\nIt may have been moved, renamed, or deleted.");
            return;
        }

        readAndReport(file);
    }

    void ProjectFileManager::clearRecentFiles()
    {
        recentFiles.clear();
        persistRecentFiles();
    }

    void ProjectFileManager::rememberRecentFile(const juce::File& file)
    {
        recentFiles.addFile(file);
        persistRecentFiles();
    }

    void ProjectFileManager::persistRecentFiles()
    {
        properties->setValue(kRecentFilesKey, recentFiles.toString());
        properties->saveIfNeeded();
    }

    void ProjectFileManager::writeAndReport(const juce::File& destination,
                                             OnSaveComplete onComplete)
    {
        if (! ProjectState::writeToFile(processor, destination))
        {
            showError("Save failed",
                      "Could not write project to:\n" + destination.getFullPathName());
            if (onComplete) onComplete(false);
            return;
        }

        // In-memory state now matches the on-disk file.
        processor.markClean();

        if (currentFile != destination)
        {
            currentFile = destination;
            if (onStateChangedCallback)
                onStateChangedCallback();
        }

        rememberRecentFile(destination);

        if (onComplete) onComplete(true);
    }

    void ProjectFileManager::readAndReport(const juce::File& source)
    {
        if (! ProjectState::readFromFile(processor, source))
        {
            showError("Open failed",
                      "Could not read project from:\n" + source.getFullPathName()
                      + "\n\nThe file may be missing, malformed, or from a future b33p version.");
            return;
        }

        currentFile = source;
        if (onStateChangedCallback)
            onStateChangedCallback();

        rememberRecentFile(source);
    }
}

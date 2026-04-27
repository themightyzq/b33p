#include "ProjectFileManager.h"

#include "B33pProcessor.h"
#include "ProjectState.h"

namespace B33p
{
    namespace
    {
        constexpr const char* kFilePattern = "*.beep";

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
    }

    ProjectFileManager::ProjectFileManager(B33pProcessor& processorRef)
        : processor(processorRef)
    {
    }

    void ProjectFileManager::setOnStateChanged(OnStateChanged callback)
    {
        onStateChangedCallback = std::move(callback);
    }

    void ProjectFileManager::save(juce::Component* parent)
    {
        if (currentFile == juce::File())
        {
            saveAs(parent);
            return;
        }
        writeAndReport(currentFile);
    }

    void ProjectFileManager::saveAs(juce::Component* parent)
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

        fileChooser->launchAsync(flags, [this, parent](const juce::FileChooser& fc)
        {
            juce::ignoreUnused(parent);
            const auto chosen = fc.getResult();
            if (chosen == juce::File())
                return;
            // Force the .beep extension if the user dropped or
            // edited it away — keeps the file-type association
            // and the Open dialog filter both predictable.
            writeAndReport(chosen.withFileExtension(".beep"));
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

    void ProjectFileManager::writeAndReport(const juce::File& destination)
    {
        if (! ProjectState::writeToFile(processor, destination))
        {
            showError("Save failed",
                      "Could not write project to:\n" + destination.getFullPathName());
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
    }
}

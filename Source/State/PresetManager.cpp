#include "PresetManager.h"

#include "B33pProcessor.h"
#include "GeneratorPresets.h"
#include "ProjectState.h"

#include <algorithm>

namespace B33p
{
    namespace
    {
        // Build the platform-appropriate path for our presets
        // directory. macOS gets ~/Library/Application Support/b33p/
        // Presets; Windows gets %APPDATA%\b33p\Presets; Linux gets
        // ~/.config/b33p/Presets. JUCE handles the platform split
        // via userApplicationDataDirectory.
        juce::File defaultPresetsDirectory()
        {
            return juce::File::getSpecialLocation(
                       juce::File::userApplicationDataDirectory)
                   .getChildFile("b33p")
                   .getChildFile("Presets");
        }
    }

    PresetManager::PresetManager(B33pProcessor& processorRef)
        : processor(processorRef),
          presetsDirectory(defaultPresetsDirectory())
    {
        // Create on construction so subsequent save / list calls
        // never have to guard against a missing root. createDirectory
        // is idempotent (returns Result::ok on an existing folder).
        presetsDirectory.createDirectory();
    }

    std::vector<juce::File> PresetManager::listPresets() const
    {
        std::vector<juce::File> presets;
        if (! presetsDirectory.isDirectory())
            return presets;

        const auto found = presetsDirectory.findChildFiles(
            juce::File::findFiles, /*searchRecursively*/ false, "*.beep");
        for (const auto& f : found)
            presets.push_back(f);

        std::sort(presets.begin(), presets.end(),
            [](const juce::File& a, const juce::File& b)
            {
                return a.getFileName().compareIgnoreCase(b.getFileName()) < 0;
            });

        return presets;
    }

    juce::File PresetManager::getPresetFile(const juce::String& name) const
    {
        // Trim whitespace and ensure we have a non-empty filename root.
        const auto trimmed = name.trim();
        if (trimmed.isEmpty())
            return {};

        // Strip any extension the user might have typed; we always append
        // `.beep` ourselves to keep listing reliable.
        const auto fileNameRoot = juce::File::createLegalFileName(
            trimmed.upToLastOccurrenceOf(".", false, false));
        if (fileNameRoot.isEmpty())
            return {};

        return presetsDirectory.getChildFile(fileNameRoot + ".beep");
    }

    juce::File PresetManager::savePreset(const juce::String& name)
    {
        // Anything that can't form a legal filename is a no-op returning an
        // invalid File so the UI can show the standard "save failed" alert.
        auto destination = getPresetFile(name);
        if (destination == juce::File())
            return {};

        if (! ProjectState::writeToFile(processor, destination))
            return {};
        return destination;
    }

    bool PresetManager::loadPreset(const juce::File& presetFile)
    {
        if (presetFile == juce::File() || ! presetFile.existsAsFile())
            return false;
        return ProjectState::readFromFile(processor, presetFile);
    }

    bool PresetManager::deletePreset(const juce::File& presetFile)
    {
        // Only delete files actually inside our presets directory,
        // and only files (not arbitrary subdirectories).
        if (! presetFile.existsAsFile())
            return false;
        if (! presetFile.isAChildOf(presetsDirectory))
            return false;
        return presetFile.deleteFile();
    }

    void PresetManager::seedFactoryPresetsIfMissing()
    {
        for (const auto& preset : getFactoryPresets())
        {
            const auto destination = presetsDirectory.getChildFile(
                juce::File::createLegalFileName(preset.name) + ".beep");
            if (destination.existsAsFile())
                continue;

            // Use a temporary B33pProcessor so the live processor's
            // state isn't disturbed during seeding. The temporary
            // never has an audio device callback attached — it's
            // purely a state holder for the save path.
            B33pProcessor temp;
            preset.configure(temp);
            ProjectState::writeToFile(temp, destination);
        }
    }

    int PresetManager::restoreFactoryPresets()
    {
        // Same as the seed path, minus the "skip if present" guard — this
        // deliberately overwrites a factory preset the user has modified,
        // so the live processor's state is again held in a throwaway temp.
        int restored = 0;
        for (const auto& preset : getFactoryPresets())
        {
            const auto destination = presetsDirectory.getChildFile(
                juce::File::createLegalFileName(preset.name) + ".beep");

            B33pProcessor temp;
            preset.configure(temp);
            if (ProjectState::writeToFile(temp, destination))
                ++restored;
        }
        return restored;
    }
}

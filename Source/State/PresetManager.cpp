#include "PresetManager.h"

#include "B33pProcessor.h"
#include "GeneratorPresets.h"
#include "PresetMigration.h"
#include "ProjectState.h"

#include <algorithm>

namespace B33p
{
    namespace
    {
        // Pre-fix per-user presets location. userApplicationDataDirectory
        // resolves to plain ~/Library on macOS (juce_Files_mac.mm's
        // userApplicationDataDirectory case is literally "~/Library", NOT
        // "~/Library/Application Support" as this file's comment used to
        // claim), so on macOS this actually landed presets at
        // ~/Library/b33p/Presets -- not the house location, and not even
        // the path every doc (README/FAQ/USAGE) described. Windows gets
        // %APPDATA%\b33p\Presets and Linux gets ~/.config/b33p/Presets (or
        // $XDG_CONFIG_HOME/b33p/Presets): both match this project's docs
        // and JUCE's own platform convention, so they are correct as-is.
        // Kept around (unconditionally, on every platform) purely as the
        // source directory for the one-time macOS migration below --
        // never written to.
        juce::File legacyPresetsDirectory()
        {
            return juce::File::getSpecialLocation(
                       juce::File::userApplicationDataDirectory)
                   .getChildFile("b33p")
                   .getChildFile("Presets");
        }

#if JUCE_MAC
        // House convention (../../CLAUDE.md section 2, "User-data folders
        // keyed on the company use ZQ SFX"): ~/Library/Audio/Presets/ZQ SFX/b33p.
        // Windows/Linux are unaffected (see legacyPresetsDirectory() above)
        // -- the house rule as written is macOS-specific, and those two
        // platforms weren't wrong to begin with.
        juce::File defaultPresetsDirectory()
        {
            return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                       .getChildFile("Library")
                       .getChildFile("Audio")
                       .getChildFile("Presets")
                       .getChildFile("ZQ SFX")
                       .getChildFile("b33p");
        }
#else
        juce::File defaultPresetsDirectory()
        {
            return legacyPresetsDirectory();
        }
#endif
    }

    PresetManager::PresetManager(B33pProcessor& processorRef)
        : processor(processorRef),
          presetsDirectory(defaultPresetsDirectory())
    {
#if JUCE_MAC
        // One-time, copy-not-move migration from the pre-fix location.
        // Idempotent (PresetMigration.h's marker file) and safe to run on
        // every launch: a no-op once migrated, a no-op with nothing
        // written when the legacy folder doesn't exist (fresh install),
        // and it never touches or deletes the legacy files. Runs before
        // the unconditional createDirectory() below so a fresh migration
        // and a fresh install both leave presetsDirectory existing either
        // way.
        PresetMigration::migrateLegacyUserPresets(legacyPresetsDirectory(), presetsDirectory);
#endif
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

    bool PresetManager::isFactoryPresetName(const juce::String& displayName) noexcept
    {
        return displayName.startsWith("Factory - ");
    }

    juce::String PresetManager::validateNewPresetName(const juce::String& candidateName,
                                                        const juce::File& beingRenamed) const
    {
        const auto trimmed = candidateName.trim();
        if (trimmed.isEmpty())
            return "Preset name can't be empty.";

        // Reject separators outright rather than silently sanitising them
        // away via createLegalFileName — a rename that quietly changes the
        // name the user typed is more confusing here than a flat refusal.
        // Check both slash styles regardless of host OS: a name typed with
        // the "wrong" platform's separator should still be rejected rather
        // than becoming a literal (and confusing) filename character.
        if (trimmed.containsChar('/') || trimmed.containsChar('\\'))
            return "Preset name can't contain path separators.";

        const auto candidateFileName = juce::File::createLegalFileName(trimmed) + ".beep";

        for (const auto& existing : listPresets())
        {
            if (existing == beingRenamed)
                continue;
            if (existing.getFileName().equalsIgnoreCase(candidateFileName))
                return "A preset named \"" + trimmed + "\" already exists.";
        }

        return {};
    }

    juce::File PresetManager::renamePreset(const juce::File& presetFile,
                                            const juce::String& candidateName)
    {
        if (! presetFile.existsAsFile() || ! presetFile.isAChildOf(presetsDirectory))
            return {};

        if (validateNewPresetName(candidateName, presetFile).isNotEmpty())
            return {};

        const auto trimmed = candidateName.trim();
        auto destination = presetsDirectory.getChildFile(
            juce::File::createLegalFileName(trimmed) + ".beep");

        // NOT `destination == presetFile`: File::operator== compares names
        // via compareIgnoreCase on macOS/Windows (juce_File.cpp), which
        // would fold a case-only rename request ("Foo" -> "foo") into this
        // "nothing to do" branch and silently drop it. Compare the raw path
        // strings exactly so only a byte-for-byte-identical destination
        // short-circuits here; a case-only difference falls through to the
        // dedicated handling below.
        if (destination.getFullPathName() == presetFile.getFullPathName())
            return presetFile;

        // Case-only rename (e.g. "Foo" -> "foo") on a case-insensitive
        // filesystem (the macOS default): the destination and source refer
        // to the same inode, so moveFileTo would see the destination as
        // already existing and refuse. Route through a temporary sibling
        // name so the OS sees two genuinely distinct moves.
        if (destination.getFullPathName().equalsIgnoreCase(presetFile.getFullPathName()))
        {
            const auto scratch = presetsDirectory.getChildFile(
                "." + juce::Uuid().toString() + ".beep_rename_tmp");
            if (! presetFile.moveFileTo(scratch))
                return {};
            if (! scratch.moveFileTo(destination))
            {
                scratch.moveFileTo(presetFile);   // best-effort restore
                return {};
            }
            return destination;
        }

        if (! presetFile.moveFileTo(destination))
            return {};

        return destination;
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

#pragma once

#include <juce_core/juce_core.h>

#include <vector>

namespace B33p
{
    class B33pProcessor;

    // Owns the per-user preset directory under the platform's
    // application-data root and exposes a small API for listing,
    // saving, loading, and deleting `.beep` preset files. The
    // browser UI builds on top of this; the main menu's Save Preset
    // / Browse Presets items also call into here.
    //
    // The directory is created on construction if missing — first
    // launch ends up with an empty Presets folder rather than a
    // missing-path error the first time the user picks
    // Save Preset.
    class PresetManager
    {
    public:
        explicit PresetManager(B33pProcessor& processor);

        // Absolute path to the per-user presets directory. Always
        // a real, on-disk folder by construction.
        juce::File getPresetsDirectory() const noexcept { return presetsDirectory; }

        // All `.beep` files inside the presets directory, sorted by
        // filename ascending. Subdirectories are NOT walked — preset
        // organisation is flat for now.
        std::vector<juce::File> listPresets() const;

        // Saves the processor's current state under the given preset
        // name. Filename becomes `<name>.beep`; existing files with
        // the same name are overwritten without prompt (the calling
        // UI is expected to confirm overwrites). Returns the saved
        // file's path on success, an invalid juce::File on failure.
        juce::File savePreset(const juce::String& name);

        // The on-disk file a given preset name would map to (same legal-name
        // + `.beep` logic savePreset uses), without writing anything. Lets the
        // UI check `.existsAsFile()` to prompt before overwriting. Returns an
        // invalid juce::File for a name that can't form a legal filename.
        juce::File getPresetFile(const juce::String& name) const;

        // Loads the named preset file into the processor. Returns
        // true on success; failure surfaces a juce::AlertWindow via
        // ProjectState::readFromFile (so the caller doesn't need to
        // double-handle the error).
        bool loadPreset(const juce::File& presetFile);

        // Deletes a preset file. No-op for files outside the
        // presets directory (defensive — never delete random files
        // even if asked).
        bool deletePreset(const juce::File& presetFile);

        // True when a preset's display name (its filename without the
        // `.beep` extension) belongs to the curated factory set — the
        // "Factory - " prefix convention defined in GeneratorPresets.cpp.
        // Single source of truth shared by the preset browser's italic-row
        // rendering and by rename gating (factory presets can't be renamed).
        static bool isFactoryPresetName(const juce::String& displayName) noexcept;

        // Validates a candidate name for renamePreset(): non-empty after
        // trimming, contains no path separators (so a rename can't escape
        // the presets directory or create a subfolder), and doesn't collide
        // case-insensitively with any other preset already on disk.
        // `beingRenamed` is excluded from the collision check so renaming a
        // preset to a name that differs from its own only by case is
        // allowed. Returns an empty string when the name is valid,
        // otherwise a short reason fit to show the user directly.
        juce::String validateNewPresetName(const juce::String& candidateName,
                                            const juce::File& beingRenamed) const;

        // Renames a user preset's on-disk file to candidateName (keeping
        // the `.beep` extension), re-validating with
        // validateNewPresetName() first. Returns the renamed file on
        // success, an invalid juce::File on failure — including when
        // presetFile isn't inside the presets directory. Factory-vs-user
        // gating is a UI-layer decision (the browser disables Rename for
        // factory rows); this check is the last line of defense, not the
        // first.
        //
        // b33p's `.beep` schema (ProjectState) has no name field of its
        // own — the display name is always derived from the filename — so
        // renaming only ever touches the file on disk, never file
        // contents.
        juce::File renamePreset(const juce::File& presetFile, const juce::String& candidateName);

        // Walks the curated list of factory presets (see
        // GeneratorPresets.h) and writes any that don't yet exist
        // on disk. Existing files are NEVER overwritten — once the
        // user has tweaked a "Factory - Delay Pad", their version
        // wins forever, even after b33p updates ship new factory
        // defaults. The user can always restore by deleting the
        // file and reopening the app.
        void seedFactoryPresetsIfMissing();

        // Re-writes EVERY factory preset to its shipped definition,
        // overwriting any the user has modified or saved over (REVIEW.md
        // P22). Destructive on name collisions — unlike the non-destructive
        // seed-on-launch path — so the caller must confirm. User presets
        // (non-factory names) are untouched. Returns the number written.
        int restoreFactoryPresets();

    private:
        B33pProcessor& processor;
        juce::File     presetsDirectory;
    };
}

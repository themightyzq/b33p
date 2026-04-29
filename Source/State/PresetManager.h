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

        // Loads the named preset file into the processor. Returns
        // true on success; failure surfaces a juce::AlertWindow via
        // ProjectState::readFromFile (so the caller doesn't need to
        // double-handle the error).
        bool loadPreset(const juce::File& presetFile);

        // Deletes a preset file. No-op for files outside the
        // presets directory (defensive — never delete random files
        // even if asked).
        bool deletePreset(const juce::File& presetFile);

    private:
        B33pProcessor& processor;
        juce::File     presetsDirectory;
    };
}

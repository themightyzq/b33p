#pragma once

#include <juce_core/juce_core.h>

#include <functional>
#include <vector>

namespace B33p
{
    class B33pProcessor;

    // Factory preset definitions. Each entry carries a display name
    // (shown in the preset browser as <name>.beep) and a configure
    // function that mutates a freshly-default-reset B33pProcessor
    // into the desired preset state. The seed-on-first-launch path
    // in PresetManager runs configure on a temporary processor,
    // serialises via ProjectState::writeToFile, and discards.
    //
    // Adding a new factory preset is just appending a new entry
    // here — no schema changes, no UI work. Existing presets the
    // user has modified are left alone (the seeder skips files
    // that already exist in the presets directory).
    struct FactoryPreset
    {
        juce::String                    name;
        std::function<void(B33pProcessor&)> configure;
    };

    // The full curated list of factory presets shipped with b33p.
    std::vector<FactoryPreset> getFactoryPresets();
}

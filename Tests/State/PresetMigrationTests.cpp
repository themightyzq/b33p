#include <catch2/catch_test_macros.hpp>

#include "State/PresetMigration.h"

#include <juce_core/juce_core.h>

using namespace B33p;

// Regression coverage for the macOS preset-folder fix (CHANGELOG.md):
// PresetManager now resolves the per-user presets directory to the house
// convention (~/Library/Audio/Presets/ZQ SFX/b33p on macOS) instead of the
// old, undocumented ~/Library/b33p/Presets, and migrates any presets a user
// already saved at the old location across on first launch. This exercises
// PresetMigration.h directly against temp directories only -- it never
// touches the real per-user folders, per CLAUDE.md's audio-thread/test
// hygiene and the task's own "inject the two directories" requirement (the
// function already takes legacyDir/newDir as plain parameters, so no
// PresetManager test seam is needed to test it in isolation).
namespace
{
    // RAII temp root so every TEST_CASE gets its own sandbox and cleans up
    // even if a REQUIRE fails partway through.
    struct TempRoot
    {
        juce::File dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                             .getChildFile("b33p_preset_migration_test_"
                                           + juce::Uuid().toString());

        TempRoot() { dir.deleteRecursively(); }
        ~TempRoot() { dir.deleteRecursively(); }
    };

    juce::File makePreset(const juce::File& parent, const juce::String& name, const juce::String& body)
    {
        parent.createDirectory();
        auto file = parent.getChildFile(name + ".beep");
        file.replaceWithText(body);
        return file;
    }

    juce::String contentsOf(const juce::File& parent, const juce::String& name)
    {
        return parent.getChildFile(name + ".beep").loadFileAsString();
    }
}

TEST_CASE("PresetMigration: a missing legacy folder copies nothing and creates no new folder",
          "[state][presets][migration]")
{
    TempRoot root;
    const auto legacy = root.dir.getChildFile("legacy/Presets");
    const auto fresh  = root.dir.getChildFile("fresh/Presets");

    REQUIRE(PresetMigration::migrateLegacyUserPresets(legacy, fresh) == 0);
    REQUIRE(! fresh.exists());
}

TEST_CASE("PresetMigration: an empty legacy folder copies nothing", "[state][presets][migration]")
{
    TempRoot root;
    const auto legacy = root.dir.getChildFile("legacy/Presets");
    const auto fresh  = root.dir.getChildFile("fresh/Presets");
    legacy.createDirectory();

    REQUIRE(PresetMigration::migrateLegacyUserPresets(legacy, fresh) == 0);
}

TEST_CASE("PresetMigration: copies every legacy preset missing from the new folder, "
          "leaves originals intact, and never overwrites an existing file",
          "[state][presets][migration]")
{
    TempRoot root;
    const auto legacy = root.dir.getChildFile("legacy/Presets");
    const auto fresh  = root.dir.getChildFile("fresh/Presets");

    makePreset(legacy, "FM Bell", "legacy-fm-bell");
    makePreset(legacy, "Resonant Stab", "legacy-resonant-stab");
    // A user preset already saved under the new location, same name as a
    // legacy one but different content -- must win over the legacy copy.
    makePreset(fresh, "Resonant Stab", "user-resonant-stab");

    const int copied = PresetMigration::migrateLegacyUserPresets(legacy, fresh);
    REQUIRE(copied == 1);   // only "FM Bell" was missing

    REQUIRE(contentsOf(fresh, "FM Bell") == "legacy-fm-bell");
    REQUIRE(contentsOf(fresh, "Resonant Stab") == "user-resonant-stab");   // not overwritten
    REQUIRE(contentsOf(legacy, "FM Bell") == "legacy-fm-bell");            // legacy left untouched
    REQUIRE(contentsOf(legacy, "Resonant Stab") == "legacy-resonant-stab");
    REQUIRE(fresh.getChildFile(PresetMigration::kMarkerFileName).existsAsFile());
}

TEST_CASE("PresetMigration: a second run is a no-op and does not resurrect a preset "
          "the user deleted from the new folder",
          "[state][presets][migration]")
{
    TempRoot root;
    const auto legacy = root.dir.getChildFile("legacy/Presets");
    const auto fresh  = root.dir.getChildFile("fresh/Presets");

    makePreset(legacy, "Delay Pad", "legacy-delay-pad");
    REQUIRE(PresetMigration::migrateLegacyUserPresets(legacy, fresh) == 1);

    // User deletes the migrated preset after the fact.
    fresh.getChildFile("Delay Pad.beep").deleteFile();

    REQUIRE(PresetMigration::migrateLegacyUserPresets(legacy, fresh) == 0);
    REQUIRE(! fresh.getChildFile("Delay Pad.beep").exists());
}

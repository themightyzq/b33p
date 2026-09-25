#pragma once

#include <juce_core/juce_core.h>

namespace B33p
{
    // One-time copy of user presets from the pre-fix per-user-data folder
    // (macOS: `~/Library/b33p/Presets` -- see PresetManager.cpp's
    // defaultPresetsDirectory() history) into the house-convention location
    // (macOS: `~/Library/Audio/Presets/ZQ SFX/b33p`, per ../../CLAUDE.md
    // section 2 "User-data folders keyed on the company").
    //
    // Header-only and juce_core-only, mirroring
    // Project_Worldizer/Source/Plugin/PresetMigration.h's shape (copy the
    // idea, not the file -- b33p never references a sibling project from
    // its build) so PresetMigrationTests.cpp can exercise it against temp
    // directories without linking the rest of the preset stack.
    //
    // b33p's presets are flat `.beep` files with no subfolders/bundles
    // (unlike Worldizer's `.wzpreset` directories), so this is a simpler,
    // flat one-level copy rather than Worldizer's recursive category-merge
    // -- adapted for b33p's data model, not copy-pasted from it.
    namespace PresetMigration
    {
        // Dropped into the new folder once migration has run, so a preset
        // the user later deletes there is not resurrected from the legacy
        // folder on a subsequent launch.
        constexpr auto kMarkerFileName = ".migrated-from-legacy-location";

        namespace detail
        {
            // Copies every FILE directly inside `from` that doesn't already
            // exist (by name) inside `to`. Files are compared by name, not
            // content -- an existing file in `to` always wins and is never
            // overwritten, even if its content differs from the legacy
            // copy. Non-file entries (there shouldn't be any in a presets
            // folder, but a stray subfolder must not crash or be silently
            // dropped from the "did everything copy?" result) are skipped
            // and do not count as a failure. Returns false if any file copy
            // failed.
            inline bool copyMissingFiles(const juce::File& from, const juce::File& to, int& copied)
            {
                bool allOk = true;
                for (const auto& entry : from.findChildFiles(juce::File::findFiles,
                                                               /*searchRecursively*/ false))
                {
                    const auto target = to.getChildFile(entry.getFileName());
                    if (target.exists())
                        continue;   // a file already in the new folder always wins

                    if (entry.copyFileTo(target))
                    {
                        ++copied;
                    }
                    else
                    {
                        allOk = false;
                        juce::Logger::writeToLog("Preset migration failed: "
                                                  + entry.getFullPathName());
                    }
                }
                return allOk;
            }
        }

        // Copies every preset in legacyDir that doesn't already exist (by
        // filename) in newDir. Copies, never moves: the legacy folder is
        // left untouched, so nothing is lost if this runs against a build
        // that still reads the old location, and nothing is lost if a copy
        // partway through fails. Idempotent via a marker file written to
        // newDir only after every file copied successfully -- a partial
        // failure leaves no marker, so the next launch retries just the
        // files that are still missing (existing successful copies are
        // skipped again via the "already exists" check, never re-copied or
        // overwritten).
        //
        // Does nothing (and writes nothing, not even the marker) when
        // legacyDir doesn't exist, which is both the normal case for a
        // fresh install and the steady state after a successful migration
        // on every launch thereafter but the first -- keeps a fresh install
        // and the common case free of filesystem writes beyond the
        // caller's own newDir.createDirectory().
        inline int migrateLegacyUserPresets(const juce::File& legacyDir, const juce::File& newDir)
        {
            if (! legacyDir.isDirectory())
                return 0;

            const auto marker = newDir.getChildFile(kMarkerFileName);
            if (marker.existsAsFile())
                return 0;

            if (! newDir.createDirectory().wasOk())
                return 0;   // couldn't even ensure the destination exists -- bail, try again next launch

            int copied = 0;
            const bool allOk = detail::copyMissingFiles(legacyDir, newDir, copied);

            // No marker on a partial copy, so the next launch retries what failed.
            if (allOk)
                marker.replaceWithText("Presets copied from " + legacyDir.getFullPathName() + "\n");

            return copied;
        }
    }
}

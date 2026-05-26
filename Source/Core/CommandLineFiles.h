#pragma once

#include <juce_core/juce_core.h>

namespace B33p
{
    // Scan a launch command line / OS document-open string for the first
    // existing `.beep` project file. Tokenises with quote handling (so a
    // quoted path containing spaces stays intact), resolves a relative path
    // against the current working directory, and skips flags or a leading
    // executable path. Returns an invalid juce::File when no existing .beep
    // is present — the common case for a plain launch.
    //
    // Lives in Core (pure juce_core, no UI) so the standalone app glue in
    // StandaloneApp.cpp and the unit suite share one implementation.
    juce::File findBeepFileInCommandLine(const juce::String& commandLine);
}

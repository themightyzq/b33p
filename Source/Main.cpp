// JUCE plugin entry point. juce_add_plugin in the CMakeLists.txt
// generates the format-specific wrappers (Standalone .app, VST3,
// AU); each one calls createPluginFilter to mint a fresh
// AudioProcessor instance.
//
// The custom Application class that used to live here (single-
// instance enforcement, quit-confirm on dirty, command-line file
// open routing) is gone — JUCE's StandaloneFilterApp wrapper
// replaces it for the .app build. Restoring those features means
// providing a custom standalone wrapper via
// JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP, which is its own follow-
// up commit.

#include <juce_audio_processors/juce_audio_processors.h>

#include "State/B33pProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new B33p::B33pProcessor();
}

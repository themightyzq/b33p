#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    // Free-floating window hosting JUCE's standard audio device
    // picker. Lets the user pick output device, sample rate, and
    // buffer size without leaving the app — the missing piece an
    // end-user reviewer flagged as "nowhere to go if the default
    // device isn't usable".
    //
    // Closes by hiding so the same instance can be re-shown later
    // without losing settings widget state.
    class AudioSettingsWindow : public juce::DocumentWindow
    {
    public:
        explicit AudioSettingsWindow(juce::AudioDeviceManager& deviceManager);

        void closeButtonPressed() override;

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsWindow)
    };
}

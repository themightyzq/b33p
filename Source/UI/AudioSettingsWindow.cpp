#include "AudioSettingsWindow.h"

namespace B33p
{
    AudioSettingsWindow::AudioSettingsWindow(juce::AudioDeviceManager& deviceManager)
        : DocumentWindow("Audio Settings",
                         juce::Colour::fromRGB(22, 22, 22),
                         DocumentWindow::closeButton)
    {
        // 0 inputs, 2 outputs — matches Application::initialise.
        // showMidi* args false because b33p doesn't accept MIDI yet.
        auto* selector = new juce::AudioDeviceSelectorComponent(
            deviceManager,
            /*minInputChannels=*/  0,
            /*maxInputChannels=*/  0,
            /*minOutputChannels=*/ 2,
            /*maxOutputChannels=*/ 2,
            /*showMidiInputs=*/    false,
            /*showMidiOutputs=*/   false,
            /*showChannelsAsStereoPairs=*/ true,
            /*hideAdvanced=*/      false);
        selector->setSize(480, 360);

        setContentOwned(selector, true);
        setUsingNativeTitleBar(true);
        setResizable(true, false);
        centreWithSize(getWidth(), getHeight());
    }

    void AudioSettingsWindow::closeButtonPressed()
    {
        setVisible(false);
    }
}

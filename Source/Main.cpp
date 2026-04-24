#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "MainComponent.h"
#include "State/B33pProcessor.h"

namespace B33p
{
    class Application : public juce::JUCEApplication
    {
    public:
        Application() = default;

        const juce::String getApplicationName() override    { return "B33p"; }
        const juce::String getApplicationVersion() override { return B33P_VERSION_STRING; }
        bool moreThanOneInstanceAllowed() override           { return true; }

        void initialise(const juce::String&) override
        {
            processor = std::make_unique<B33pProcessor>();

            // 0 inputs, 2 outputs, default device, no stored XML.
            deviceManager = std::make_unique<juce::AudioDeviceManager>();
            deviceManager->initialiseWithDefaultDevices(0, 2);

            processorPlayer = std::make_unique<juce::AudioProcessorPlayer>();
            processorPlayer->setProcessor(processor.get());
            deviceManager->addAudioCallback(processorPlayer.get());

            mainWindow = std::make_unique<MainWindow>(getApplicationName(), *processor);
        }

        void shutdown() override
        {
            if (deviceManager != nullptr && processorPlayer != nullptr)
                deviceManager->removeAudioCallback(processorPlayer.get());

            processorPlayer.reset();
            deviceManager.reset();
            mainWindow.reset();
            processor.reset();
        }

        void systemRequestedQuit() override        { quit(); }
        void anotherInstanceStarted(const juce::String&) override {}

    private:
        class MainWindow : public juce::DocumentWindow
        {
        public:
            MainWindow(const juce::String& name, B33pProcessor& processorRef)
                : DocumentWindow(name,
                                 juce::Desktop::getInstance()
                                     .getDefaultLookAndFeel()
                                     .findColour(juce::ResizableWindow::backgroundColourId),
                                 DocumentWindow::allButtons)
            {
                setUsingNativeTitleBar(true);
                setContentOwned(new MainComponent(processorRef), true);
                setResizable(true, false);
                centreWithSize(getWidth(), getHeight());
                setVisible(true);
            }

            void closeButtonPressed() override
            {
                JUCEApplication::getInstance()->systemRequestedQuit();
            }

        private:
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
        };

        std::unique_ptr<B33pProcessor>              processor;
        std::unique_ptr<juce::AudioDeviceManager>   deviceManager;
        std::unique_ptr<juce::AudioProcessorPlayer> processorPlayer;
        std::unique_ptr<MainWindow>                 mainWindow;
    };
}

START_JUCE_APPLICATION(B33p::Application)

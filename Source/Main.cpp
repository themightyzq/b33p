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

        const juce::String getApplicationName() override    { return "b33p"; }
        const juce::String getApplicationVersion() override { return B33P_VERSION_STRING; }

        // Single-instance enforcement: when a second copy is
        // launched (e.g. user double-clicks a .beep file in
        // Finder), JUCE routes the request to the running instance
        // via anotherInstanceStarted() rather than spawning a
        // duplicate. Standard document-app convention on macOS.
        bool moreThanOneInstanceAllowed() override           { return false; }

        void initialise(const juce::String& commandLine) override
        {
            processor = std::make_unique<B33pProcessor>();

            // 0 inputs, 2 outputs, default device, no stored XML.
            deviceManager = std::make_unique<juce::AudioDeviceManager>();
            deviceManager->initialiseWithDefaultDevices(0, 2);

            processorPlayer = std::make_unique<juce::AudioProcessorPlayer>();
            processorPlayer->setProcessor(processor.get());
            deviceManager->addAudioCallback(processorPlayer.get());

            mainWindow = std::make_unique<MainWindow>(getApplicationName(), *processor);

            handleCommandLineFile(commandLine);
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

        // Re-routes "open with file" requests from the OS into the
        // already-running instance. On macOS this fires when the
        // user double-clicks a registered .beep file in Finder
        // while b33p is already open; on Linux/Windows it fires
        // when a second copy is launched with the file as an
        // argument.
        void anotherInstanceStarted(const juce::String& commandLine) override
        {
            handleCommandLineFile(commandLine);
        }

    private:
        void handleCommandLineFile(const juce::String& commandLine)
        {
            const auto path = commandLine.trim().unquoted();
            if (path.isEmpty())
                return;

            const juce::File file(path);
            if (file.existsAsFile() && file.hasFileExtension(".beep"))
            {
                if (mainWindow != nullptr)
                    if (auto* mc = mainWindow->getMainComponent())
                        mc->openProjectFile(file);
            }
        }

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
                auto* component = new MainComponent(processorRef);
                mainComponent = component;
                setContentOwned(component, true);
                setResizable(true, false);
                centreWithSize(getWidth(), getHeight());
                setVisible(true);
            }

            void closeButtonPressed() override
            {
                JUCEApplication::getInstance()->systemRequestedQuit();
            }

            // Non-owning typed pointer kept so the Application can
            // route OS-driven file-open events into the right
            // component. The window owns the lifetime via
            // setContentOwned; mainComponent is invalidated only by
            // a subsequent setContentOwned call which we don't do.
            MainComponent* getMainComponent() const noexcept { return mainComponent; }

        private:
            MainComponent* mainComponent { nullptr };

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
        };

        std::unique_ptr<B33pProcessor>              processor;
        std::unique_ptr<juce::AudioDeviceManager>   deviceManager;
        std::unique_ptr<juce::AudioProcessorPlayer> processorPlayer;
        std::unique_ptr<MainWindow>                 mainWindow;
    };
}

START_JUCE_APPLICATION(B33p::Application)

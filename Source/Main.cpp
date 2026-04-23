#include <juce_gui_basics/juce_gui_basics.h>

#include "MainComponent.h"

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
            mainWindow = std::make_unique<MainWindow>(getApplicationName());
        }

        void shutdown() override
        {
            mainWindow.reset();
        }

        void systemRequestedQuit() override        { quit(); }
        void anotherInstanceStarted(const juce::String&) override {}

    private:
        class MainWindow : public juce::DocumentWindow
        {
        public:
            explicit MainWindow(const juce::String& name)
                : DocumentWindow(name,
                                 juce::Desktop::getInstance()
                                     .getDefaultLookAndFeel()
                                     .findColour(juce::ResizableWindow::backgroundColourId),
                                 DocumentWindow::allButtons)
            {
                setUsingNativeTitleBar(true);
                setContentOwned(new MainComponent(), true);
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

        std::unique_ptr<MainWindow> mainWindow;
    };
}

START_JUCE_APPLICATION(B33p::Application)

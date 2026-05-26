// Custom standalone application for the b33p .app build.
//
// JUCE's juce_add_plugin normally supplies its own StandaloneFilterApp
// (in juce_audio_plugin_client_Standalone.cpp). Defining
// JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1 (see CMakeLists.txt) compiles
// that default out and hands the standalone entry point to the
// juce_CreateApplication() defined at the bottom of this file instead.
//
// Why we need it: the default wrapper quits the moment the user closes
// the window or presses Cmd+Q, silently discarding unsaved work. The
// File ▸ New / Open paths already guard against that via
// MainComponent::confirmDiscardThen — this restores the same prompt for
// the window-close and quit gestures, which only the wrapper can see.
// (Listed under TODO.md "Deferred regressions".)
//
// Single-instance enforcement and command-line .beep open are the other
// two deferred regressions; this app deliberately keeps JUCE's default
// behaviour for both (moreThanOneInstanceAllowed() → true, no-op
// anotherInstanceStarted) so this change stays scoped to quit-confirm.

#include <juce_audio_processors/juce_audio_processors.h>

#if JucePlugin_Build_Standalone

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

#include "MainComponent.h"
#include "State/B33pProcessor.h"
#include "UI/B33pEditor.h"

namespace B33p
{
    namespace
    {
        // Walk the standalone window down to the MainComponent that owns
        // the confirm dialog + ProjectFileManager. Returns nullptr if the
        // editor hasn't been created or isn't ours (e.g. a generic editor
        // fallback), in which case callers quit without prompting.
        MainComponent* findMainComponent(juce::StandaloneFilterWindow& window)
        {
            if (auto* processor = window.getAudioProcessor())
                if (auto* editor = processor->getActiveEditor())
                    if (auto* b33pEditor = dynamic_cast<B33pEditor*>(editor))
                        return &b33pEditor->getMainComponent();

            return nullptr;
        }

        // Shared close / quit handler. Clean projects quit immediately
        // (identical to JUCE's default). Dirty projects route through the
        // existing Save / Discard / Cancel prompt; the app only quits on
        // Save (success) or Discard. savePluginState() still runs on the
        // way out so the patch + window position restore on next launch.
        void confirmThenQuit(juce::StandaloneFilterWindow* window)
        {
            auto doQuit = [windowPtr = juce::Component::SafePointer<juce::StandaloneFilterWindow>(window)]()
            {
                if (auto* w = windowPtr.getComponent())
                    w->pluginHolder->savePluginState();

                juce::JUCEApplicationBase::quit();
            };

            auto* mainComponent = window != nullptr ? findMainComponent(*window) : nullptr;
            auto* processor     = window != nullptr
                                    ? dynamic_cast<B33pProcessor*>(window->getAudioProcessor())
                                    : nullptr;

            if (mainComponent == nullptr || processor == nullptr || ! processor->isDirty())
            {
                doQuit();
                return;
            }

            // Don't stack a second prompt if one is already showing (e.g.
            // the user clicks the close button and then presses Cmd+Q).
            // The modal count drops back to zero when the dialog closes, so
            // a Cancel leaves the next quit attempt free to prompt again.
            if (juce::ModalComponentManager::getInstance()->getNumModalComponents() > 0)
                return;

            mainComponent->confirmDiscardThen(doQuit);
        }

        // The shipping window. Everything except the close-button override
        // is inherited from JUCE's StandaloneFilterWindow unchanged.
        class StandaloneWindow final : public juce::StandaloneFilterWindow
        {
        public:
            using juce::StandaloneFilterWindow::StandaloneFilterWindow;

            void closeButtonPressed() override
            {
                confirmThenQuit(this);
            }
        };

        // Mirrors JUCE's (now-compiled-out) StandaloneFilterApp, with two
        // changes: createWindow() makes our StandaloneWindow, and
        // systemRequestedQuit() runs the same confirm-on-dirty path so
        // Cmd+Q / OS-quit behaves like the close button.
        class StandaloneApp final : public juce::JUCEApplication
        {
        public:
            StandaloneApp()
            {
                juce::PropertiesFile::Options options;
                options.applicationName     = juce::CharPointer_UTF8(JucePlugin_Name);
                options.filenameSuffix      = ".settings";
                options.osxLibrarySubFolder = "Application Support";
               #if JUCE_LINUX || JUCE_BSD
                options.folderName          = "~/.config";
               #else
                options.folderName          = "";
               #endif

                appProperties.setStorageParameters(options);
            }

            const juce::String getApplicationName() override    { return juce::CharPointer_UTF8(JucePlugin_Name); }
            const juce::String getApplicationVersion() override { return JucePlugin_VersionString; }
            bool moreThanOneInstanceAllowed() override          { return true; }
            void anotherInstanceStarted(const juce::String&) override {}

            void initialise(const juce::String&) override
            {
                mainWindow = createWindow();

                if (mainWindow != nullptr)
                    mainWindow->setVisible(true);
                else
                    pluginHolder = createPluginHolder();   // headless fallback (no display)
            }

            void shutdown() override
            {
                pluginHolder = nullptr;
                mainWindow   = nullptr;
                appProperties.saveIfNeeded();
            }

            void systemRequestedQuit() override
            {
                if (pluginHolder != nullptr)
                    pluginHolder->savePluginState();

                confirmThenQuit(mainWindow.get());
            }

        private:
            std::unique_ptr<juce::StandalonePluginHolder> createPluginHolder()
            {
                const juce::Array<juce::StandalonePluginHolder::PluginInOuts> channelConfig;

                return std::make_unique<juce::StandalonePluginHolder>(
                    appProperties.getUserSettings(),
                    false,
                    juce::String{},
                    nullptr,
                    channelConfig,
                    false);
            }

            std::unique_ptr<StandaloneWindow> createWindow()
            {
                if (juce::Desktop::getInstance().getDisplays().displays.isEmpty())
                {
                    jassertfalse;   // no display: fall back to a headless holder
                    return nullptr;
                }

                auto window = std::make_unique<StandaloneWindow>(
                    getApplicationName(),
                    juce::LookAndFeel::getDefaultLookAndFeel()
                        .findColour(juce::ResizableWindow::backgroundColourId),
                    createPluginHolder());

                // Fit-to-screen on launch. The editor opens at its natural
                // size; on a display smaller than that (e.g. a 1080p laptop)
                // JUCE's StandaloneFilterWindow centres at full size without
                // clamping, so part of the window lands off-screen. Shrink to
                // the display's usable area and re-centre. The window stays
                // resizable, so the user can grow it back afterward.
                const auto& displays = juce::Desktop::getInstance().getDisplays();
                if (auto* display = displays.getDisplayForRect(window->getBounds()))
                {
                    const auto usable = display->userArea;
                    const auto bounds = window->getBounds();
                    const int  w      = juce::jmin(bounds.getWidth(),  usable.getWidth());
                    const int  h      = juce::jmin(bounds.getHeight(), usable.getHeight());

                    if (w < bounds.getWidth() || h < bounds.getHeight())
                        window->setBounds(juce::Rectangle<int>(w, h).withCentre(usable.getCentre()));
                }

                return window;
            }

            juce::ApplicationProperties                   appProperties;
            std::unique_ptr<StandaloneWindow>             mainWindow;
            std::unique_ptr<juce::StandalonePluginHolder> pluginHolder;
        };
    } // namespace
} // namespace B33p

// Hands b33p's standalone entry point to our custom app. The JUCE
// standalone wrapper (compiled in the B33p_Standalone target) declares
// this extern and calls it from main().
JUCE_CREATE_APPLICATION_DEFINE(B33p::StandaloneApp)

#endif // JucePlugin_Build_Standalone

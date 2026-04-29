#pragma once

#include "State/B33pProcessor.h"
#include "State/ProjectFileManager.h"
#include "UI/AmpEnvSection.h"
#include "UI/AudioSettingsWindow.h"
#include "UI/EffectsSection.h"
#include "UI/FilterSection.h"
#include "UI/MasterSection.h"
#include "UI/ModEffectsSection.h"
#include "UI/ModulationSection.h"
#include "UI/OscillatorSection.h"
#include "UI/PatternSection.h"
#include "UI/PitchEnvSection.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    class MainComponent : public juce::Component
                        , public juce::MenuBarModel
                        , public juce::KeyListener
    {
    public:
        // deviceManager is a non-owning reference held by the
        // application. MainComponent uses it for the Audio Settings
        // dialog only — it never adds/removes audio callbacks.
        MainComponent(B33pProcessor& processor,
                      juce::AudioDeviceManager& deviceManager);
        ~MainComponent() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        bool keyPressed(const juce::KeyPress& key) override;

        // KeyListener — registered on the top-level window so
        // transport keys (Space, Shift+Space) work even when no
        // component has focus. TextEditor focus is treated as
        // "user is typing, don't intercept".
        bool keyPressed(const juce::KeyPress& key,
                        juce::Component* originatingComponent) override;

        // OS-driven file open (Finder double-click, dock drop).
        // Routes to ProjectFileManager so failure surfaces the
        // same alert as the dialog-driven Open command.
        void openProjectFile(const juce::File& file);

        // Shows a Save / Discard / Cancel prompt if the project is
        // dirty, then runs `proceed` only if the user picked Save
        // (and the save succeeded) or Discard. Cancel is a no-op.
        // If the project is already clean, `proceed` runs immediately.
        // Used by the close-button / quit handler to prevent
        // accidental loss of unsaved work.
        void confirmDiscardThen(std::function<void()> proceed);

        // juce::MenuBarModel
        juce::StringArray getMenuBarNames() override;
        juce::PopupMenu   getMenuForIndex(int topLevelIndex,
                                          const juce::String& menuName) override;
        void              menuItemSelected(int menuItemId,
                                           int topLevelIndex) override;

    private:
        void parentHierarchyChanged() override;
        void updateWindowTitle();
        void showAboutDialog();
        void showAudioSettings();

        // The top-level component we registered the KeyListener on,
        // kept so the destructor can deregister cleanly even after
        // the window is mid-teardown.
        juce::Component::SafePointer<juce::Component> keyListenerHost;

        B33pProcessor&             processor;
        juce::AudioDeviceManager&  deviceManager;
        ProjectFileManager         fileManager;

        std::unique_ptr<AudioSettingsWindow> audioSettingsWindow;

        // Owns the hover-tooltip popup for the whole window. Just
        // declaring it is enough — every Component with a non-empty
        // tooltip text gets picked up automatically.
        juce::TooltipWindow tooltipWindow { this, 700 };

        // In-window menu bar — kept in the window rather than
        // setMacMainMenu so the layout is identical on every OS
        // and we don't need an #ifdef __APPLE__ here.
        juce::MenuBarComponent menuBar { this };

        OscillatorSection oscillatorSection;
        AmpEnvSection     ampEnvelopeSection;
        FilterSection     filterSection;
        EffectsSection    effectsSection;
        ModEffectsSection modEffectsSection;
        ModulationSection modulationSection;
        MasterSection     masterSection;
        PitchEnvSection   pitchEnvelopeSection;
        PatternSection    patternSection;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
    };
}

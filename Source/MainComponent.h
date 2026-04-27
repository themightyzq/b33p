#pragma once

#include "State/B33pProcessor.h"
#include "State/ProjectFileManager.h"
#include "UI/AmpEnvSection.h"
#include "UI/EffectsSection.h"
#include "UI/FilterSection.h"
#include "UI/MasterSection.h"
#include "UI/OscillatorSection.h"
#include "UI/PatternSection.h"
#include "UI/PitchEnvSection.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    class MainComponent : public juce::Component
    {
    public:
        explicit MainComponent(B33pProcessor& processor);
        ~MainComponent() override = default;

        void paint(juce::Graphics& g) override;
        void resized() override;

        bool keyPressed(const juce::KeyPress& key) override;

        // OS-driven file open (Finder double-click, dock drop).
        // Routes to ProjectFileManager so failure surfaces the
        // same alert as the dialog-driven Open command.
        void openProjectFile(const juce::File& file);

    private:
        void parentHierarchyChanged() override;
        void updateWindowTitle();
        void showAboutDialog();

        B33pProcessor&      processor;
        ProjectFileManager  fileManager;

        OscillatorSection oscillatorSection;
        AmpEnvSection     ampEnvelopeSection;
        FilterSection     filterSection;
        EffectsSection    effectsSection;
        MasterSection     masterSection;
        PitchEnvSection   pitchEnvelopeSection;
        PatternSection    patternSection;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
    };
}

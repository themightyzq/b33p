#pragma once

#include "MainComponent.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace B33p
{
    class B33pProcessor;

    // Thin AudioProcessorEditor wrapper around MainComponent. The
    // editor sits on top of an AudioProcessor as JUCE expects;
    // MainComponent does all the actual UI work. Resizable so the
    // pattern grid doesn't get cropped when the host window starts
    // small.
    class B33pEditor : public juce::AudioProcessorEditor
    {
    public:
        explicit B33pEditor(B33pProcessor& processor);

        void resized() override;

        // The standalone wrapper reaches the unsaved-changes prompt
        // (MainComponent::confirmDiscardThen) through this when the
        // user closes the window or quits — see Source/StandaloneApp.cpp.
        MainComponent& getMainComponent() noexcept { return mainComponent; }

    private:
        MainComponent mainComponent;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(B33pEditor)
    };
}

#pragma once

#include "IconButton.h"
#include "LabeledSlider.h"
#include "Section.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace B33p
{
    class OscillatorSection : public Section
    {
    public:
        explicit OscillatorSection(B33pProcessor& processor);

        void resized() override;

        void retargetLane(int lane);

    private:

        B33pProcessor& processor;

        juce::ComboBox waveformSelector;
        IconButton     waveformDice { IconButton::Glyph::Die  };
        IconButton     waveformLock { IconButton::Glyph::Lock };

        LabeledSlider    basePitchSlider { "Pitch" };

        // unique_ptr so retargetLane can swap them out.
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   basePitchAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorSection)
    };
}

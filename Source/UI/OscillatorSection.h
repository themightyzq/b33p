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

    private:
        juce::ComboBox waveformSelector;
        IconButton     waveformDice { IconButton::Glyph::Die  };
        IconButton     waveformLock { IconButton::Glyph::Lock };

        LabeledSlider    basePitchSlider { "Pitch" };

        // ComboBoxAttachment is unique_ptr because it must be
        // constructed after the ComboBox has its items populated —
        // the attachment's initial parameterChanged pushes into the
        // combo, which would be a no-op on an empty item list.
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
        juce::AudioProcessorValueTreeState::SliderAttachment                    basePitchAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorSection)
    };
}

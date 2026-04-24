#pragma once

#include "LabeledSlider.h"
#include "Section.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace B33p
{
    class MasterSection : public Section
    {
    public:
        explicit MasterSection(juce::AudioProcessorValueTreeState& apvts);

        void resized() override;

    private:
        LabeledSlider gainSlider { "Gain" };

        juce::AudioProcessorValueTreeState::SliderAttachment gainAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterSection)
    };
}

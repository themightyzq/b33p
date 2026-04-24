#pragma once

#include "LabeledSlider.h"
#include "Section.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace B33p
{
    class FilterSection : public Section
    {
    public:
        explicit FilterSection(juce::AudioProcessorValueTreeState& apvts);

        void resized() override;

    private:
        LabeledSlider cutoffSlider     { "Cutoff" };
        LabeledSlider resonanceSlider  { "Q"      };

        juce::AudioProcessorValueTreeState::SliderAttachment cutoffAttachment;
        juce::AudioProcessorValueTreeState::SliderAttachment resonanceAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterSection)
    };
}

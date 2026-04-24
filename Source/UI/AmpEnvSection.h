#pragma once

#include "AmpEnvelopeVisualizer.h"
#include "LabeledSlider.h"
#include "Section.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace B33p
{
    class AmpEnvSection : public Section
    {
    public:
        explicit AmpEnvSection(juce::AudioProcessorValueTreeState& apvts);

        void resized() override;

    private:
        LabeledSlider attackSlider  { "Attack"  };
        LabeledSlider decaySlider   { "Decay"   };
        LabeledSlider sustainSlider { "Sustain" };
        LabeledSlider releaseSlider { "Release" };

        AmpEnvelopeVisualizer visualizer;

        juce::AudioProcessorValueTreeState::SliderAttachment attackAttachment;
        juce::AudioProcessorValueTreeState::SliderAttachment decayAttachment;
        juce::AudioProcessorValueTreeState::SliderAttachment sustainAttachment;
        juce::AudioProcessorValueTreeState::SliderAttachment releaseAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmpEnvSection)
    };
}

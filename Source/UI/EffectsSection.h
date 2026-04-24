#pragma once

#include "LabeledSlider.h"
#include "Section.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace B33p
{
    class EffectsSection : public Section
    {
    public:
        explicit EffectsSection(juce::AudioProcessorValueTreeState& apvts);

        void resized() override;

    private:
        LabeledSlider bitDepthSlider   { "Bits"  };
        LabeledSlider crushRateSlider  { "Rate"  };
        LabeledSlider driveSlider      { "Drive" };

        juce::AudioProcessorValueTreeState::SliderAttachment bitDepthAttachment;
        juce::AudioProcessorValueTreeState::SliderAttachment crushRateAttachment;
        juce::AudioProcessorValueTreeState::SliderAttachment driveAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsSection)
    };
}

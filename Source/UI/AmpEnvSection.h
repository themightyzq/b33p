#pragma once

#include "AmpEnvelopeVisualizer.h"
#include "LabeledSlider.h"
#include "Section.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace B33p
{
    class AmpEnvSection : public Section
    {
    public:
        explicit AmpEnvSection(B33pProcessor& processor);

        void resized() override;

        void retargetLane(int lane);

    private:

        B33pProcessor& processor;

        LabeledSlider attackSlider  { "Attack"  };
        LabeledSlider decaySlider   { "Decay"   };
        LabeledSlider sustainSlider { "Sustain" };
        LabeledSlider releaseSlider { "Release" };

        AmpEnvelopeVisualizer visualizer;

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmpEnvSection)
    };
}

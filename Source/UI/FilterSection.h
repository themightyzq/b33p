#pragma once

#include "FilterResponseVisualizer.h"
#include "LabeledSlider.h"
#include "Section.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace B33p
{
    class FilterSection : public Section
    {
    public:
        explicit FilterSection(B33pProcessor& processor);

        void resized() override;

        void retargetLane(int lane);

    private:

        B33pProcessor& processor;

        FilterResponseVisualizer visualizer;
        LabeledSlider cutoffSlider    { "Cutoff"    };
        LabeledSlider resonanceSlider { "Resonance" };

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonanceAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterSection)
    };
}

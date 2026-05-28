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
                        , private juce::Timer
    {
    public:
        explicit FilterSection(B33pProcessor& processor);

        void resized() override;

        void retargetLane(int lane);

    private:
        void onTypeChanged();
        // Modulation-glow driver — reads the selected-lane LFO mirror
        // values + the matrix slot config, writes a per-knob intensity
        // that drives the halo painted by `B33pLookAndFeel`.
        void timerCallback() override;

        B33pProcessor& processor;
        int            currentLane { 0 };

        FilterResponseVisualizer visualizer;
        juce::ComboBox typeSelector;
        LabeledSlider  cutoffSlider    { "Cutoff"    };
        LabeledSlider  resonanceSlider { "Resonance" };
        LabeledSlider  vowelSlider     { "Vowel"     };

        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   cutoffAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   resonanceAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   vowelAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterSection)
    };
}

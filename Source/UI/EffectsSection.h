#pragma once

#include "LabeledSlider.h"
#include "Section.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace B33p
{
    class EffectsSection : public Section
                         , private juce::Timer
    {
    public:
        explicit EffectsSection(B33pProcessor& processor);

        void resized() override;

        void retargetLane(int lane);

    private:
        void timerCallback() override;

        B33pProcessor& processor;
        int            currentLane { 0 };

        LabeledSlider bitDepthSlider  { "Bits"  };
        LabeledSlider crushRateSlider { "Rate"  };
        LabeledSlider driveSlider     { "Drive" };

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bitDepthAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crushRateAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsSection)
    };
}

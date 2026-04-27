#pragma once

#include "LabeledSlider.h"
#include "Section.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace B33p
{
    class MasterSection : public Section
                        , private juce::Timer
    {
    public:
        explicit MasterSection(B33pProcessor& processor);

        void resized() override;

    private:
        void timerCallback() override;
        void flashAuditionButton();

        LabeledSlider    gainSlider     { "Gain" };
        juce::TextButton auditionButton { "Audition" };
        juce::TextButton diceAllButton  { "Dice All" };

        juce::AudioProcessorValueTreeState::SliderAttachment gainAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterSection)
    };
}

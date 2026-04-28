#pragma once

#include "LabeledSlider.h"
#include "Section.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace B33p
{
    class MasterSection : public Section
                        , private juce::Timer
    {
    public:
        explicit MasterSection(B33pProcessor& processor);

        void resized() override;

        void retargetLane(int lane);

    private:
        void timerCallback() override;
        void flashAuditionButton();

        B33pProcessor& processor;

        // Gain explicitly opts out of the randomizer — too easy
        // to blast the user with a random +20 dB jump otherwise.
        LabeledSlider    gainSlider     { "Gain", /*showRandomizer=*/ false };
        juce::TextButton auditionButton { "Audition" };
        juce::TextButton diceAllButton  { "Dice All" };

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterSection)
    };
}

#pragma once

#include "PatternGrid.h"
#include "Section.h"
#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    class PatternSection : public Section
                          , private juce::Timer
    {
    public:
        explicit PatternSection(B33pProcessor& processor);
        ~PatternSection() override;

        void resized() override;

    private:
        void timerCallback() override;

        B33pProcessor&   processor;
        PatternGrid      grid;
        juce::TextButton playButton { "Play" };
        juce::TextButton loopToggle { "Loop" };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternSection)
    };
}

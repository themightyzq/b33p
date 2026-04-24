#pragma once

#include "PatternGrid.h"
#include "Section.h"
#include "State/B33pProcessor.h"

namespace B33p
{
    class PatternSection : public Section
    {
    public:
        explicit PatternSection(B33pProcessor& processor);

        void resized() override;

    private:
        PatternGrid grid;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternSection)
    };
}

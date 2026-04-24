#include "PatternSection.h"

namespace B33p
{
    PatternSection::PatternSection(B33pProcessor& processor)
        : Section("Pattern"),
          grid(processor)
    {
        addAndMakeVisible(grid);
    }

    void PatternSection::resized()
    {
        grid.setBounds(getContentBounds());
    }
}

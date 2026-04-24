#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    // Titled panel that groups a related cluster of controls.
    // Draws a rounded outline and a small header with the title
    // above an empty content area. Subclasses or callers lay out
    // child controls inside getContentBounds().
    class Section : public juce::Component
    {
    public:
        explicit Section(juce::String title);

        void paint(juce::Graphics& g) override;

        // The inner rectangle children should use — excludes the
        // outline, title strip, and padding.
        juce::Rectangle<int> getContentBounds() const;

    private:
        juce::String title;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Section)
    };
}

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

        // Tail tag appended to the title (e.g. " — Lane 1: Body").
        // Used by per-lane sections to telegraph which lane the
        // controls are currently editing. Empty by default.
        void setTitleSuffix(const juce::String& suffix);

    private:
        juce::String title;
        juce::String suffix;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Section)
    };
}

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    // Titled panel that groups a related cluster of controls. Draws the house
    // zqsfx::ui::Panel treatment (hard-edged rectangle, panelTop/panelBot gradient
    // face, 1 px panelBorder, faint top inner highlight, silkTitle text above a
    // ruleTitle hairline — docs/ZQSFX_UI_STYLE_GUIDE.md section 6) with a small
    // header with the title above an empty content area. Subclasses or callers
    // lay out child controls inside getContentBounds().
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

        // Optional 2 px coloured strip painted just below the title
        // bar. Used by per-lane voice sections to show which lane
        // they're targeting at a glance. Pass juce::Colour() to
        // suppress.
        void setAccentColour(juce::Colour c);

        // The four pattern lanes as the colour-blind-safe channels, in style-guide
        // table order (section 3: "b33p: lanes 1 to 4: comp.sky, comp.yellow,
        // comp.purple, comp.white"). This is the house-token replacement for
        // B33pProcessor::laneAccentColour(int) — the per-lane colour VALUES moved
        // here (a UI-owned helper) because Source/State/B33pProcessor.cpp is out of
        // scope for this migration.
        // Every call site that used to read processor.laneAccentColour(lane) now
        // reads this instead; the processor's own method is unchanged and unused
        // by the UI.
        static juce::Colour houseLaneAccent(int lane);

    private:
        juce::String title;
        juce::String suffix;
        juce::Colour accentColour;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Section)
    };
}

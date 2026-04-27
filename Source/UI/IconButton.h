#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    // Tiny vector-icon button. Paints either a die (for "roll") or a
    // padlock (for "lock") using juce::Path primitives — crisp at any
    // DPI, no PNG/SVG asset to ship.
    //
    // Lock state is driven by juce::Button::getToggleState(); when the
    // owner sets clickingTogglesState(true), the lock auto-toggles
    // open / closed and the paint reflects it without any extra
    // wiring.
    class IconButton : public juce::Button
    {
    public:
        enum class Glyph { Die, Lock };

        explicit IconButton(Glyph glyph);

        void paintButton(juce::Graphics& g,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    private:
        void paintDie (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour ink);
        void paintLock(juce::Graphics& g, juce::Rectangle<float> area,
                       juce::Colour ink, bool locked);

        Glyph glyph;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IconButton)
    };
}

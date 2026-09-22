#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <zqsfx_ui/zqsfx_ui.h>

namespace B33p
{
    // The house LookAndFeel's drawLcdText()/lcdFont() are non-static (they read the
    // loaded font typefaces), so text drawn on a phosphor screen has to go through
    // whatever LookAndFeel is currently installed (always B33pLookAndFeel, a
    // zqsfx::ui::LookAndFeel subclass, in this plugin). Falls back to a plain
    // generic-font drawText if that is ever not the case (e.g. a component
    // momentarily under a different LookAndFeel, or the headless snapshot tool
    // before the editor installs its own), so this never crashes or draws nothing.
    // Mirrors LFlOw's LfoDisplay.cpp drawScreenText helper (same migration pattern).
    inline void drawHouseScreenText(juce::Component& c, juce::Graphics& g, const juce::String& text,
                                    juce::Rectangle<int> area, float px,
                                    juce::Justification just = juce::Justification::centred,
                                    juce::Colour colour = zqsfx::ui::colour::lcdText)
    {
        if (auto* houseLnF = dynamic_cast<zqsfx::ui::LookAndFeel*>(&c.getLookAndFeel()))
            houseLnF->drawLcdText(g, text, area, px, just, colour);
        else
        {
            g.setColour(colour);
            g.setFont(juce::FontOptions(px));
            g.drawText(text, area, just);
        }
    }
}

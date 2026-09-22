#include "Section.h"

#include <zqsfx_ui/zqsfx_ui.h>

namespace B33p
{
    namespace
    {
        constexpr int   kTitleStripHeight = 22;
        constexpr int   kTitleIndent      = 10;
        // House inset convention: 6 px inside a titled panel, 8 px left under
        // the last control row (asymmetric on purpose — see the worked
        // examples' migration reports).
        constexpr int   kContentInset        = 6;
        constexpr int   kContentBottomInset  = 8;

        // Push the lane accent down onto every slider in the subtree so
        // LabeledSlider's meaning-colour chip (see LabeledSlider::paint) and
        // any bespoke linear-slider fill (B33pLookAndFeel::drawLinearSlider)
        // paint in the current lane's colour — matching the section's accent
        // strip. Safe to call on lane switches; sliders with no rotary/track
        // colour usage simply ignore the colour IDs. The house filmstrip
        // rotary knob itself does not consult these IDs (it always draws the
        // same knob art regardless of dial size), which is why the lane
        // meaning colour lives on the label chip and the section strip
        // instead of the knob face — see docs/ZQSFX_UI_STYLE_GUIDE.md and the
        // migration report's "never colour alone" table.
        void tintSliders(juce::Component& component, juce::Colour accent)
        {
            for (auto* child : component.getChildren())
            {
                if (child == nullptr)
                    continue;

                if (auto* slider = dynamic_cast<juce::Slider*>(child))
                {
                    slider->setColour(juce::Slider::rotarySliderFillColourId, accent);
                    slider->setColour(juce::Slider::trackColourId,            accent);
                    slider->setColour(juce::Slider::thumbColourId,            accent);
                }

                tintSliders(*child, accent);
            }
        }
    }

    Section::Section(juce::String titleIn)
        : title(std::move(titleIn))
    {
    }

    juce::Colour Section::houseLaneAccent(int lane)
    {
        // Table order per style guide section 3: sky, yellow, purple, white.
        return zqsfx::ui::comp::channel(lane);
    }

    void Section::paint(juce::Graphics& g)
    {
        // House zqsfx::ui::Panel treatment (Panel.h), reproduced here rather than
        // composed because Section already owns getContentBounds()/the title-suffix/
        // accent-strip mechanism this product relies on. Hard-edged rectangle, no
        // rounded corners (style guide section 6).
        namespace colour = zqsfx::ui::colour;
        auto bounds = getLocalBounds().toFloat();

        g.setGradientFill(zqsfx::ui::gradients::panel(bounds));
        g.fillRect(bounds);
        g.setColour(juce::Colours::white.withAlpha(0.04f));   // inner top highlight
        g.fillRect(bounds.withHeight(1.0f).translated(0.0f, 1.0f));
        g.setColour(colour::panelBorder);
        g.drawRect(bounds, 1.0f);

        g.setColour(colour::silkTitle);
        g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)).withExtraKerningFactor(0.27f));

        auto titleArea = getLocalBounds()
                             .removeFromTop(kTitleStripHeight)
                             .withTrimmedLeft(kTitleIndent)
                             .withTrimmedRight(kTitleIndent);
        g.drawText((title + suffix).toUpperCase(), titleArea, juce::Justification::centredLeft);

        g.setColour(colour::ruleTitle);
        g.fillRect(juce::Rectangle<int>(kTitleIndent, kTitleStripHeight,
                                         getWidth() - 2 * kTitleIndent, 1));

        // Per-section accent strip just under the title hairline — visual
        // cue for which lane this section currently edits. 4 px at full
        // alpha so it carries across the editor without becoming heavy.
        if (accentColour.getAlpha() > 0)
        {
            const auto strip = juce::Rectangle<int>(
                getLocalBounds().getX() + kTitleIndent,
                kTitleStripHeight + 1,
                getLocalBounds().getWidth() - 2 * kTitleIndent,
                4);
            g.setColour(accentColour);
            g.fillRect(strip);
        }
    }

    void Section::setTitleSuffix(const juce::String& s)
    {
        if (suffix == s) return;
        suffix = s;
        repaint();
    }

    void Section::setAccentColour(juce::Colour c)
    {
        if (accentColour == c) return;
        accentColour = c;
        tintSliders(*this, c);
        repaint();
    }

    juce::Rectangle<int> Section::getContentBounds() const
    {
        auto bounds = getLocalBounds().withTrimmedTop(kTitleStripHeight);
        bounds.removeFromLeft(kContentInset);
        bounds.removeFromRight(kContentInset);
        bounds.removeFromTop(kContentInset);
        bounds.removeFromBottom(kContentBottomInset);
        return bounds;
    }
}

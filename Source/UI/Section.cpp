#include "Section.h"

namespace B33p
{
    namespace
    {
        constexpr int   kCornerRadius    = 6;
        constexpr int   kOutlineInset    = 1;
        constexpr int   kTitleStripHeight = 22;
        constexpr int   kTitleIndent     = 10;
        constexpr int   kContentPadding  = 8;
        constexpr float kOutlineThickness = 1.0f;

        // Push the lane accent down onto every slider in the subtree so the
        // custom LookAndFeel paints each knob's value arc / fill in the
        // current lane's colour — matching the section's accent strip. Safe
        // to call on lane switches; non-rotary sliders simply ignore the
        // rotary colour IDs.
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

    void Section::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat()
                          .reduced(static_cast<float>(kOutlineInset));

        g.setColour(juce::Colour::fromRGB(36, 36, 36));
        g.fillRoundedRectangle(bounds, static_cast<float>(kCornerRadius));

        g.setColour(juce::Colour::fromRGB(90, 90, 90));
        g.drawRoundedRectangle(bounds,
                               static_cast<float>(kCornerRadius),
                               kOutlineThickness);

        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));

        auto titleArea = getLocalBounds()
                             .removeFromTop(kTitleStripHeight)
                             .withTrimmedLeft(kTitleIndent)
                             .withTrimmedRight(kTitleIndent);
        g.drawText(title + suffix, titleArea, juce::Justification::centredLeft);

        // Per-section accent strip just under the title bar — visual
        // cue for which lane this section currently edits. 4 px at
        // full alpha so it carries across the editor without becoming
        // heavy; the 2 px / α 0.85 version was almost invisible at
        // normal viewing distance.
        if (accentColour.getAlpha() > 0)
        {
            const auto strip = juce::Rectangle<int>(
                getLocalBounds().getX() + kCornerRadius,
                kTitleStripHeight,
                getLocalBounds().getWidth() - 2 * kCornerRadius,
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
        return getLocalBounds()
                   .withTrimmedTop(kTitleStripHeight)
                   .reduced(kContentPadding);
    }
}

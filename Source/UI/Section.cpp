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
        g.drawText(title, titleArea, juce::Justification::centredLeft);
    }

    juce::Rectangle<int> Section::getContentBounds() const
    {
        return getLocalBounds()
                   .withTrimmedTop(kTitleStripHeight)
                   .reduced(kContentPadding);
    }
}

#include "B33pLookAndFeel.h"

#include <cmath>

namespace B33p
{
    B33pLookAndFeel::B33pLookAndFeel()
    {
        namespace colour = zqsfx::ui::colour;

        // The base zqsfx::ui::LookAndFeel constructor already sets the house colours
        // this class used to set itself: ComboBox/PopupMenu -> LCD glass, Slider
        // textbox -> LCD glass + glow, TextButton -> btn gradient / accent-on,
        // TooltipWindow/AlertWindow/TextEditor -> house tokens.
        //
        // Default (non-lane) slider colour: most sliders in this product get their
        // rotarySliderFillColourId/trackColourId set per-lane by Section::tintSliders
        // whenever a Section's accent changes. A few controls live outside any
        // per-lane Section (the modulation-matrix amount sliders' bipolar fill still
        // routes through this default when a slot is unrouted at construction, the
        // pattern section's randomize-scope slider, the event-overrides dialog's
        // sliders) — for those, default to the house LCD green rather than JUCE's
        // stock blue, so an un-tinted control still reads as "this product's screen
        // content" instead of a leftover default theme colour.
        setColour(juce::Slider::rotarySliderFillColourId, colour::lcdText);
        setColour(juce::Slider::trackColourId,             colour::lcdText);
        setColour(juce::Slider::thumbColourId,             colour::lcdText);
    }

    bool B33pLookAndFeel::isBipolar(const juce::Slider& s)
    {
        return s.getMinimum() < 0.0
            && juce::approximatelyEqual(s.getMinimum(), -s.getMaximum());
    }

    juce::Font B33pLookAndFeel::getSliderPopupFont(juce::Slider&)
    {
        return lcdFont(15.0f);
    }

    void B33pLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float minSliderPos, float maxSliderPos,
                                           juce::Slider::SliderStyle style, juce::Slider& slider)
    {
        if (style != juce::Slider::LinearHorizontal)
        {
            juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                                   minSliderPos, maxSliderPos, style, slider);
            return;
        }

        namespace colour = zqsfx::ui::colour;

        const bool  enabled  = slider.isEnabled();
        const float alphaMul = enabled ? 1.0f : zqsfx::ui::geom::dimAlpha;
        const auto  bounds   = juce::Rectangle<int>(x, y, width, height).toFloat();

        // Screen-glass track, ruleTitle border — hard rectangle, no rounded caps
        // (style guide section 6: replace rounded corners with hard edges).
        constexpr float trackH = 4.0f;
        const float trackY = bounds.getCentreY() - trackH * 0.5f;
        const juce::Rectangle<float> track(bounds.getX(), trackY, bounds.getWidth(), trackH);
        g.setColour(colour::lcdScreenDark.withAlpha(alphaMul));
        g.fillRect(track);
        g.setColour(colour::ruleTitle.withAlpha(alphaMul));
        g.drawRect(track, 1.0f);

        // Filled portion, in the control's own colour (rotarySliderFillColourId /
        // trackColourId — set per-lane by Section::tintSliders, or this class's own
        // lcdText default otherwise).
        const auto fillColour = slider.findColour(juce::Slider::trackColourId);

        if (isBipolar(slider))
        {
            const float centreX = bounds.getX() + bounds.getWidth() * 0.5f;
            const auto  fill    = juce::Rectangle<float>::leftTopRightBottom(
                juce::jmin(centreX, sliderPos), track.getY(),
                juce::jmax(centreX, sliderPos), track.getBottom());
            g.setColour(fillColour.withAlpha(alphaMul));
            g.fillRect(fill);

            g.setColour(colour::ruleInner.withAlpha(alphaMul));
            g.fillRect(juce::Rectangle<float>(centreX - 0.5f, track.getY() - 2.0f,
                                              1.0f, track.getHeight() + 4.0f));
        }
        else
        {
            const float fillW = juce::jlimit(0.0f, bounds.getWidth(), sliderPos - bounds.getX());
            if (fillW > 0.0f)
            {
                const juce::Rectangle<float> fill(bounds.getX(), trackY, fillW, trackH);
                g.setColour(fillColour.withAlpha(alphaMul));
                g.fillRect(fill);
            }
        }

        // Slim rectangular thumb — no stock ball, no rounded capsule.
        constexpr float thumbW = 5.0f;
        constexpr float thumbH = 14.0f;
        const juce::Rectangle<float> thumb(sliderPos - thumbW * 0.5f, bounds.getCentreY() - thumbH * 0.5f,
                                           thumbW, thumbH);
        g.setColour(colour::pointer.withAlpha(alphaMul));
        g.fillRect(thumb);
    }
}

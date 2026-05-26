#include "B33pLookAndFeel.h"

#include <cmath>

namespace B33p
{
    namespace
    {
        const juce::Colour kWindow   = juce::Colour::fromRGB(22, 22, 22);
        const juce::Colour kPanel    = juce::Colour::fromRGB(34, 34, 38);
        const juce::Colour kWidget   = juce::Colour::fromRGB(46, 46, 51);
        const juce::Colour kKnobBody = juce::Colour::fromRGB(28, 28, 31);
        const juce::Colour kTrack    = juce::Colour::fromRGB(64, 64, 70);
        const juce::Colour kOutline  = juce::Colour::fromRGB(72, 72, 80);
        // Default accent — the per-lane tint overrides this on most knobs.
        const juce::Colour kAccent   = juce::Colour::fromRGB(120, 200, 255);
    }

    B33pLookAndFeel::B33pLookAndFeel()
    {
        using LF = juce::LookAndFeel_V4;

        setColour(juce::ResizableWindow::backgroundColourId, kWindow);

        setColour(juce::PopupMenu::backgroundColourId,            kPanel);
        setColour(juce::PopupMenu::textColourId,                  juce::Colours::white);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, kAccent.withAlpha(0.22f));
        setColour(juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);

        setColour(juce::ComboBox::backgroundColourId, kWidget);
        setColour(juce::ComboBox::textColourId,       juce::Colours::white);
        setColour(juce::ComboBox::outlineColourId,    kOutline);
        setColour(juce::ComboBox::arrowColourId,      kAccent);
        setColour(juce::ComboBox::buttonColourId,     kWidget);

        setColour(juce::TextButton::buttonColourId,   kWidget);
        setColour(juce::TextButton::buttonOnColourId, kAccent);
        setColour(juce::TextButton::textColourOnId,   juce::Colours::black);
        setColour(juce::TextButton::textColourOffId,  juce::Colours::white);

        setColour(juce::Slider::rotarySliderFillColourId,    kAccent);
        setColour(juce::Slider::rotarySliderOutlineColourId, kTrack);
        setColour(juce::Slider::trackColourId,               kAccent);
        setColour(juce::Slider::backgroundColourId,          kTrack);
        setColour(juce::Slider::thumbColourId,               kAccent);
        setColour(juce::Slider::textBoxTextColourId,         juce::Colours::white);
        setColour(juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);

        setColour(juce::Label::textColourId, juce::Colours::white);

        setColour(juce::AlertWindow::backgroundColourId, kPanel);
        setColour(juce::AlertWindow::textColourId,       juce::Colours::white);
        setColour(juce::AlertWindow::outlineColourId,    kOutline);

        // Keep the V4 colour scheme broadly consistent for any widgets we
        // don't draw by hand (text editors, scrollbars, list boxes).
        setColourScheme(LF::getDarkColourScheme());
    }

    bool B33pLookAndFeel::isBipolar(const juce::Slider& s)
    {
        return s.getMinimum() < 0.0
            && juce::approximatelyEqual(s.getMinimum(), -s.getMaximum());
    }

    void B33pLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float startAngle, float endAngle,
                                           juce::Slider& slider)
    {
        const auto area   = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(3.0f);
        const auto centre = area.getCentre();
        const float radius   = juce::jmin(area.getWidth(), area.getHeight()) * 0.5f;
        const float lineW    = juce::jmax(2.0f, radius * 0.12f);
        const float arcR     = radius - lineW * 0.5f;
        const bool  enabled  = slider.isEnabled();

        auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);
        if (! enabled) accent = accent.withAlpha(0.30f);

        // Flat knob body — a subtle disc darker than the panel, no bevel.
        g.setColour(kKnobBody.withAlpha(enabled ? 1.0f : 0.6f));
        g.fillEllipse(juce::Rectangle<float>(arcR * 1.55f, arcR * 1.55f).withCentre(centre));

        // Background track ring (full sweep).
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour(kTrack.withAlpha(enabled ? 1.0f : 0.5f));
        g.strokePath(track, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        // Value arc — from the start (or the centre, if bipolar) to the value.
        const float toAngle   = startAngle + sliderPos * (endAngle - startAngle);
        const float fromAngle = isBipolar(slider) ? (startAngle + endAngle) * 0.5f : startAngle;
        const float a0 = juce::jmin(fromAngle, toAngle);
        const float a1 = juce::jmax(fromAngle, toAngle);

        if (a1 - a0 > 0.001f)
        {
            juce::Path value;
            value.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f, a0, a1, true);
            g.setColour(accent);
            g.strokePath(value, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        }

        // Indicator dot riding the arc at the current value (clockwise from top).
        const juce::Point<float> dot(centre.x + arcR * std::sin(toAngle),
                                     centre.y - arcR * std::cos(toAngle));
        g.setColour(accent.brighter(0.35f));
        g.fillEllipse(juce::Rectangle<float>(lineW * 1.5f, lineW * 1.5f).withCentre(dot));
    }

    void B33pLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                           juce::Slider::SliderStyle style, juce::Slider& slider)
    {
        if (style != juce::Slider::LinearHorizontal)
        {
            juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                                   0.0f, 0.0f, style, slider);
            return;
        }

        const bool enabled = slider.isEnabled();
        auto accent = slider.findColour(juce::Slider::trackColourId);
        if (! enabled) accent = accent.withAlpha(0.30f);

        const float cy   = (float) y + (float) height * 0.5f;
        const auto  track = juce::Rectangle<float>((float) x, cy - 2.0f, (float) width, 4.0f);

        g.setColour(kTrack.withAlpha(enabled ? 1.0f : 0.5f));
        g.fillRoundedRectangle(track, 2.0f);

        if (isBipolar(slider))
        {
            const float centreX = (float) x + (float) width * 0.5f;
            const auto fill = juce::Rectangle<float>::leftTopRightBottom(
                juce::jmin(centreX, sliderPos), track.getY(),
                juce::jmax(centreX, sliderPos), track.getBottom());
            g.setColour(accent);
            g.fillRoundedRectangle(fill, 2.0f);

            // Centre detent tick.
            g.setColour(juce::Colour::fromRGB(96, 96, 104));
            g.fillRect(juce::Rectangle<float>(centreX - 0.5f, track.getY() - 2.0f, 1.0f,
                                              track.getHeight() + 4.0f));
        }
        else
        {
            const auto fill = juce::Rectangle<float>::leftTopRightBottom(
                (float) x, track.getY(), sliderPos, track.getBottom());
            g.setColour(accent);
            g.fillRoundedRectangle(fill, 2.0f);
        }

        g.setColour(accent.brighter(0.35f));
        g.fillEllipse(juce::Rectangle<float>(11.0f, 11.0f).withCentre({ sliderPos, cy }));
    }

    void B33pLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                       int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                       juce::ComboBox& box)
    {
        const auto bounds  = juce::Rectangle<float>(0.0f, 0.0f, (float) width, (float) height).reduced(1.0f);
        const bool enabled = box.isEnabled();

        g.setColour(box.findColour(juce::ComboBox::backgroundColourId)
                        .withAlpha(enabled ? 1.0f : 0.5f));
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(box.findColour(juce::ComboBox::outlineColourId).withAlpha(enabled ? 1.0f : 0.4f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Accent chevron caret.
        const auto zone = juce::Rectangle<float>((float) width - 18.0f, 0.0f, 14.0f, (float) height);
        const float cx = zone.getCentreX();
        const float cy = zone.getCentreY();
        juce::Path caret;
        caret.startNewSubPath(cx - 4.0f, cy - 2.0f);
        caret.lineTo(cx, cy + 3.0f);
        caret.lineTo(cx + 4.0f, cy - 2.0f);
        g.setColour(box.findColour(juce::ComboBox::arrowColourId).withAlpha(enabled ? 1.0f : 0.3f));
        g.strokePath(caret, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    void B33pLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                               const juce::Colour& backgroundColour,
                                               bool shouldDrawButtonAsHighlighted,
                                               bool shouldDrawButtonAsDown)
    {
        const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);

        auto fill = backgroundColour;
        if (shouldDrawButtonAsDown)            fill = fill.darker(0.25f);
        else if (shouldDrawButtonAsHighlighted) fill = fill.brighter(0.18f);

        g.setColour(fill);
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(kOutline.withAlpha(0.6f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    void B33pLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
    {
        g.fillAll(findColour(juce::PopupMenu::backgroundColourId));
        g.setColour(kOutline);
        g.drawRect(0, 0, width, height, 1);
    }

    juce::Font B33pLookAndFeel::getSliderPopupFont(juce::Slider&)
    {
        return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                            13.0f, juce::Font::plain));
    }

    juce::Label* B33pLookAndFeel::createSliderTextBox(juce::Slider& slider)
    {
        auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
        label->setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
                                                    11.0f, juce::Font::plain)));
        return label;
    }
}

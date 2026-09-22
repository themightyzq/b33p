#include "IconButton.h"

#include <zqsfx_ui/zqsfx_ui.h>

namespace B33p
{
    namespace
    {
        namespace houseColour = zqsfx::ui::colour;

        // House button/legend tokens (style guide section 6: "Hover turns the
        // legend accent"). Lock in toggle-on state also takes the accent colour
        // so it reads as "active" at a glance — the same rule as any other lit
        // toggle.
        const juce::Colour& kIdleInk      = houseColour::btnText;
        const juce::Colour& kHoverInk     = houseColour::accent;
        const juce::Colour& kPressedInk   = houseColour::accent;
        const juce::Colour& kLockedAccent = houseColour::accent;
        const juce::Colour& kButtonBgIdle  = houseColour::btnBot;
        const juce::Colour& kButtonBgHover = houseColour::btnTop;
    }

    namespace
    {
        juce::String glyphName(IconButton::Glyph g)
        {
            switch (g)
            {
                case IconButton::Glyph::Die:          return "Roll";
                case IconButton::Glyph::Lock:         return "Lock";
                case IconButton::Glyph::ChevronLeft:  return "Previous";
                case IconButton::Glyph::ChevronRight: return "Next";
            }
            return "Button";
        }
    }

    IconButton::IconButton(Glyph g)
        : juce::Button(glyphName(g)),
          glyph(g)
    {
        // Accessibility floor: accessible title always set (style guide
        // section 8); chevrons get a context-specific tooltip/description
        // from the owner (the preset prev/next caption) instead of the
        // generic ones below.
        setTitle(glyphName(g));
        if (g == Glyph::Die)
        {
            setTooltip("Roll a random value");
            setDescription("Roll a random value for this parameter");
        }
        else if (g == Glyph::Lock)
        {
            setTooltip("Lock to exclude from random rolls");
            setDescription("Lock this parameter to exclude it from random rolls");
        }
    }

    void IconButton::paintButton(juce::Graphics& g,
                                  bool shouldDrawButtonAsHighlighted,
                                  bool shouldDrawButtonAsDown)
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        // Background — flat, subtle, hard-edged (style guide section 6: no
        // rounded corners). Slightly brighter on hover.
        const auto bg = shouldDrawButtonAsHighlighted ? kButtonBgHover
                                                      : kButtonBgIdle;
        g.setColour(bg);
        g.fillRect(bounds);
        g.setColour(houseColour::btnBorder);
        g.drawRect(bounds, 1.0f);

        // Pick the foreground tone. Lock in toggle-on state takes the
        // accent colour so it reads as "active" at a glance.
        juce::Colour ink = kIdleInk;
        if (shouldDrawButtonAsDown)        ink = kPressedInk;
        else if (shouldDrawButtonAsHighlighted) ink = kHoverInk;

        if (glyph == Glyph::Lock && getToggleState())
            ink = kLockedAccent;

        // Inset the icon area so it doesn't touch the button edges.
        const auto iconArea = bounds.reduced(bounds.getWidth() * 0.18f,
                                             bounds.getHeight() * 0.18f);

        switch (glyph)
        {
            case Glyph::Die:          paintDie(g, iconArea, ink); break;
            case Glyph::Lock:         paintLock(g, iconArea, ink, getToggleState()); break;
            case Glyph::ChevronLeft:  paintChevron(g, iconArea, ink, false); break;
            case Glyph::ChevronRight: paintChevron(g, iconArea, ink, true);  break;
        }
    }

    void IconButton::paintChevron(juce::Graphics& g,
                                  juce::Rectangle<float> area,
                                  juce::Colour ink,
                                  bool pointRight)
    {
        const float side = std::min(area.getWidth(), area.getHeight());
        const auto  cell = juce::Rectangle<float>(side, side).withCentre(area.getCentre());

        // Two arms meeting at an apex on the pointing side — a ">" or "<".
        const float midY  = cell.getCentreY();
        const float apexX = pointRight ? cell.getRight() - side * 0.30f
                                       : cell.getX()     + side * 0.30f;
        const float baseX = pointRight ? cell.getX()     + side * 0.34f
                                       : cell.getRight() - side * 0.34f;
        const float topY  = cell.getY()      + side * 0.24f;
        const float botY  = cell.getBottom() - side * 0.24f;

        juce::Path chevron;
        chevron.startNewSubPath(baseX, topY);
        chevron.lineTo(apexX, midY);
        chevron.lineTo(baseX, botY);

        g.setColour(ink);
        g.strokePath(chevron, juce::PathStrokeType(1.8f,
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    void IconButton::paintDie(juce::Graphics& g,
                              juce::Rectangle<float> area,
                              juce::Colour ink)
    {
        // Square outline + quincunx (5-pip) face.
        const float side  = std::min(area.getWidth(), area.getHeight());
        const auto  square = juce::Rectangle<float>(side, side)
                                 .withCentre(area.getCentre());

        g.setColour(ink);
        g.drawRoundedRectangle(square, side * 0.18f, 1.5f);

        const float r = side * 0.12f;     // pip radius
        const float pX[] = { 0.28f, 0.72f, 0.50f, 0.28f, 0.72f };
        const float pY[] = { 0.28f, 0.28f, 0.50f, 0.72f, 0.72f };
        for (int i = 0; i < 5; ++i)
        {
            const float cx = square.getX() + pX[i] * square.getWidth();
            const float cy = square.getY() + pY[i] * square.getHeight();
            g.fillEllipse(cx - r, cy - r, 2.0f * r, 2.0f * r);
        }
    }

    void IconButton::paintLock(juce::Graphics& g,
                               juce::Rectangle<float> area,
                               juce::Colour ink,
                               bool locked)
    {
        // Pad shape: a chunky body on the bottom 60% with a shackle
        // arc on the top 50%. Open lock pivots the shackle right
        // off its left foot for the unlocked state.
        const float side = std::min(area.getWidth(), area.getHeight());
        const auto  cell = juce::Rectangle<float>(side, side)
                              .withCentre(area.getCentre());

        const float bodyW = cell.getWidth() * 0.78f;
        const float bodyH = cell.getHeight() * 0.50f;
        const auto  body  = juce::Rectangle<float>(bodyW, bodyH)
                                .withCentre({ cell.getCentreX(),
                                              cell.getY() + cell.getHeight() * 0.72f });

        const float shackleW = bodyW * 0.62f;
        const float shackleH = bodyH * 0.95f;
        const auto  shackleBounds = juce::Rectangle<float>(shackleW, shackleH)
                                       .withCentre({ cell.getCentreX(),
                                                     body.getY() - shackleH * 0.18f });

        g.setColour(ink);

        juce::Path shackle;
        if (locked)
        {
            // Closed shackle: U pointing down with both feet meeting
            // the top of the body.
            shackle.startNewSubPath(shackleBounds.getX(), body.getY());
            shackle.lineTo(shackleBounds.getX(), shackleBounds.getCentreY());
            shackle.addCentredArc(shackleBounds.getCentreX(),
                                  shackleBounds.getCentreY(),
                                  shackleBounds.getWidth() * 0.5f,
                                  shackleBounds.getHeight() * 0.5f,
                                  0.0f,
                                  -juce::MathConstants<float>::halfPi,
                                  juce::MathConstants<float>::halfPi);
            shackle.lineTo(shackleBounds.getRight(), body.getY());
        }
        else
        {
            // Open shackle: only the left foot touches the body; the
            // right foot pivots up and away.
            shackle.startNewSubPath(shackleBounds.getX(), body.getY());
            shackle.lineTo(shackleBounds.getX(), shackleBounds.getCentreY());
            shackle.addCentredArc(shackleBounds.getCentreX(),
                                  shackleBounds.getCentreY(),
                                  shackleBounds.getWidth() * 0.5f,
                                  shackleBounds.getHeight() * 0.5f,
                                  0.0f,
                                  -juce::MathConstants<float>::halfPi,
                                  juce::MathConstants<float>::halfPi);
            shackle.lineTo(shackleBounds.getRight(),
                           shackleBounds.getY() + shackleBounds.getHeight() * 0.45f);
        }
        g.strokePath(shackle, juce::PathStrokeType(1.7f,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        // Body: rounded rect, filled when locked, outlined when not.
        if (locked)
        {
            g.fillRoundedRectangle(body, body.getHeight() * 0.18f);
        }
        else
        {
            g.drawRoundedRectangle(body, body.getHeight() * 0.18f, 1.5f);
        }

        // Tiny keyhole dot (only when locked, against the filled body).
        if (locked)
        {
            const float kr = body.getHeight() * 0.16f;
            g.setColour(houseColour::btnBot);
            g.fillEllipse(body.getCentreX() - kr,
                          body.getCentreY() - kr,
                          2.0f * kr, 2.0f * kr);
        }
    }
}

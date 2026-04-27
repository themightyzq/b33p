#include "IconButton.h"

namespace B33p
{
    namespace
    {
        // Tones tuned against the section background (rgb 36,36,36).
        const juce::Colour kIdleInk        { juce::Colour::fromRGB(150, 150, 150) };
        const juce::Colour kHoverInk       { juce::Colour::fromRGB(220, 220, 220) };
        const juce::Colour kPressedInk     { juce::Colour::fromRGB(255, 255, 255) };
        const juce::Colour kLockedAccent   { juce::Colour::fromRGB(120, 200, 255) };
        const juce::Colour kButtonBgIdle   { juce::Colour::fromRGB( 48,  48,  48) };
        const juce::Colour kButtonBgHover  { juce::Colour::fromRGB( 64,  64,  64) };

        constexpr float kButtonCorner = 3.0f;
    }

    IconButton::IconButton(Glyph g)
        : juce::Button(g == Glyph::Die ? "Roll" : "Lock"),
          glyph(g)
    {
        setTooltip(g == Glyph::Die
                       ? juce::String("Roll a random value")
                       : juce::String("Lock to exclude from random rolls"));
    }

    void IconButton::paintButton(juce::Graphics& g,
                                  bool shouldDrawButtonAsHighlighted,
                                  bool shouldDrawButtonAsDown)
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        // Background — flat, subtle. Slightly brighter on hover.
        const auto bg = shouldDrawButtonAsHighlighted ? kButtonBgHover
                                                      : kButtonBgIdle;
        g.setColour(bg);
        g.fillRoundedRectangle(bounds, kButtonCorner);

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

        if (glyph == Glyph::Die)
            paintDie(g, iconArea, ink);
        else
            paintLock(g, iconArea, ink, getToggleState());
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
        g.drawRoundedRectangle(square, side * 0.18f, 1.2f);

        const float r = side * 0.10f;     // pip radius
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
        g.strokePath(shackle, juce::PathStrokeType(1.4f,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        // Body: rounded rect, filled when locked, outlined when not.
        if (locked)
        {
            g.fillRoundedRectangle(body, body.getHeight() * 0.18f);
        }
        else
        {
            g.drawRoundedRectangle(body, body.getHeight() * 0.18f, 1.2f);
        }

        // Tiny keyhole dot (only when locked, against the filled body).
        if (locked)
        {
            const float kr = body.getHeight() * 0.16f;
            g.setColour(juce::Colour::fromRGB(36, 36, 36));
            g.fillEllipse(body.getCentreX() - kr,
                          body.getCentreY() - kr,
                          2.0f * kr, 2.0f * kr);
        }
    }
}

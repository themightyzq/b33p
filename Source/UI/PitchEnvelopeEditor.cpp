#include "PitchEnvelopeEditor.h"

#include "HouseScreenText.h"
#include "State/UndoableActions.h"

#include <zqsfx_ui/zqsfx_ui.h>

#include <algorithm>

namespace B33p
{
    namespace
    {
        constexpr float kSemitoneRange = 12.0f;
        constexpr float kHitRadius     = 10.0f;
        constexpr float kPlotInset     = 6.0f;
        constexpr float kPointRadius   = 5.0f;
        constexpr float kCurveStroke   = 2.0f;

        void sortByTime(std::vector<PitchEnvelopePoint>& curve)
        {
            std::stable_sort(curve.begin(), curve.end(),
                [](const PitchEnvelopePoint& a, const PitchEnvelopePoint& b)
                {
                    return a.normalizedTime < b.normalizedTime;
                });
        }
    }

    PitchEnvelopeEditor::PitchEnvelopeEditor(B33pProcessor& processorRef)
        : processor(processorRef)
    {
        // Crosshair telegraphs "click here to add a point" on hover.
        // Without it the canvas reads as a passive display, not an
        // editable surface — and the empty-state hint becomes the
        // only signal that the area is interactive.
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
        setAccessible(true);
        setTitle("Pitch envelope curve");
        setDescription("Pitch envelope breakpoint curve, shared across all lanes. "
                       "Click to add a point, drag to shape, right-click a point to delete.");
    }

    juce::Rectangle<float> PitchEnvelopeEditor::plotArea() const
    {
        return getLocalBounds().toFloat().reduced(kPlotInset);
    }

    juce::Point<float> PitchEnvelopeEditor::toScreen(const PitchEnvelopePoint& p) const
    {
        const auto area = plotArea();
        const float t  = juce::jlimit(0.0f, 1.0f, p.normalizedTime);
        const float st = juce::jlimit(-kSemitoneRange, kSemitoneRange, p.semitones);
        const float x  = area.getX() + t * area.getWidth();
        const float y  = area.getCentreY() - (st / kSemitoneRange) * (area.getHeight() / 2.0f);
        return { x, y };
    }

    PitchEnvelopePoint PitchEnvelopeEditor::fromScreen(juce::Point<float> screen) const
    {
        const auto area = plotArea();
        const float t  = (screen.x - area.getX()) / area.getWidth();
        const float st = -(screen.y - area.getCentreY()) / (area.getHeight() / 2.0f) * kSemitoneRange;
        return { juce::jlimit(0.0f, 1.0f, t),
                 juce::jlimit(-kSemitoneRange, kSemitoneRange, st) };
    }

    int PitchEnvelopeEditor::hitTestPoint(juce::Point<float> screen) const
    {
        const auto& curve = processor.getPitchCurve();
        int   bestIdx  = -1;
        float bestDist = kHitRadius;
        for (size_t i = 0; i < curve.size(); ++i)
        {
            const float d = toScreen(curve[i]).getDistanceFrom(screen);
            if (d < bestDist)
            {
                bestDist = d;
                bestIdx  = static_cast<int>(i);
            }
        }
        return bestIdx;
    }

    void PitchEnvelopeEditor::paint(juce::Graphics& g)
    {
        namespace houseColour = zqsfx::ui::colour;
        const auto frame = getLocalBounds().toFloat().reduced(1.0f);

        // Phosphor screen treatment (style guide section 6).
        zqsfx::ui::LookAndFeel::drawScreen(g, frame, false);

        const auto area = plotArea();
        const auto& curve = processor.getPitchCurve();

        // Zero-semitones baseline. Brighter when empty (acts as the
        // visible reference axis the user will be drawing relative to);
        // dimmer when there's an actual curve to avoid competing with it.
        const auto baselineColour = curve.empty() ? houseColour::lcdFaint : houseColour::lcdFaint2;
        g.setColour(baselineColour);
        g.drawHorizontalLine(static_cast<int>(area.getCentreY()),
                             area.getX(), area.getRight());

        // Axis reference (P32). Faint vertical gridlines at the quarter
        // points of the note's duration, plus semitone labels down the left
        // edge, so the curve reads against concrete pitch values (±12 st)
        // instead of unitless space. ASCII labels only.
        g.setColour(houseColour::lcdFaint2);
        for (const float t : { 0.25f, 0.5f, 0.75f })
            g.drawVerticalLine(static_cast<int>(area.getX() + t * area.getWidth()),
                               area.getY(), area.getBottom());

        // At the editor's minimum window height, three fixed-pixel rows above
        // this one (Oscillator/AmpEnv/Filter, Effects/Master/ModFX, plus the
        // menu/header/padding) can squeeze this shared bottom row to well
        // under 40 px tall, leaving under 20 px for the three stacked "+12" /
        // "0" / "-12" labels — not enough room for any legible font, and
        // VT323 in particular renders visibly taller than its point size
        // implies (a pixel-styled face), so the labels overlapped each other
        // and the hint text at that floor size (verified against the AFTER
        // render at 1000x600). Rather than fight for an illegible font size,
        // skip the axis numerals below this height; the curve and baseline
        // still draw either way.
        if (area.getHeight() >= 40.0f)
        {
            const auto label = [&](const char* text, float cy)
            {
                drawHouseScreenText(*this, g, text,
                           juce::Rectangle<int>(static_cast<int>(area.getX()) + 2,
                                                static_cast<int>(cy) - 6, 26, 12),
                           9.0f, juce::Justification::centredLeft, houseColour::lcdFaint);
            };
            label("+12", area.getY() + 6.0f);
            label("0",   area.getCentreY());
            label("-12", area.getBottom() - 6.0f);
        }

        // First-run hint — shown when the curve is effectively flat (the
        // default state is two boundary points at 0 semitones, which
        // draws as the zero baseline above and looks like "nothing's
        // here" to a fresh user). The hint disappears the moment any
        // point has a non-zero semitone value OR the user has added a
        // third point. (REVIEW-USER M-MISSING-3 — the empty-state hint
        // existed but its old `curve.empty()` condition never triggered
        // because the default curve has two zero-pitch points.)
        const bool curveIsFlat = curve.size() <= 2 && std::all_of(
            curve.begin(), curve.end(),
            [](const PitchEnvelopePoint& p) { return std::abs(p.semitones) < 1e-3f; });
        if (curve.empty() || curveIsFlat)
        {
            drawHouseScreenText(*this, g,
                       "Click anywhere to add a pitch point. Drag to shape; right-click a point to delete.",
                       area.toNearestInt(), 12.0f, juce::Justification::centred, houseColour::lcdFaint);
            return;
        }

        std::vector<PitchEnvelopePoint> sorted(curve);
        sortByTime(sorted);

        // Path: flat lead-in at first point's value, linear interp
        // between consecutive points, flat trail-out at last value.
        // Mirrors PitchEnvelope's runtime behaviour for curves that
        // do not span [0, 1].
        juce::Path path;
        const auto first = toScreen(sorted.front());
        path.startNewSubPath(area.getX(), first.y);
        path.lineTo(first);
        for (size_t i = 1; i < sorted.size(); ++i)
            path.lineTo(toScreen(sorted[i]));
        const auto last = toScreen(sorted.back());
        path.lineTo(area.getRight(), last.y);

        // This curve is shared across all lanes (not per-lane data), so it takes
        // the default phosphor-screen content colour (lcdText) rather than any
        // one lane's channel colour or the accent (which would misread as
        // "active" / "this is lane N's data").
        g.setColour(houseColour::lcdText);
        g.strokePath(path, juce::PathStrokeType(kCurveStroke));

        for (const auto& p : sorted)
        {
            const auto screen = toScreen(p);
            g.setColour(houseColour::lcdText);
            g.fillEllipse(screen.x - kPointRadius, screen.y - kPointRadius,
                          kPointRadius * 2.0f, kPointRadius * 2.0f);
            g.setColour(houseColour::lcdBg);
            g.drawEllipse(screen.x - kPointRadius, screen.y - kPointRadius,
                          kPointRadius * 2.0f, kPointRadius * 2.0f, 1.5f);
        }
    }

    void PitchEnvelopeEditor::mouseDown(const juce::MouseEvent& e)
    {
        // Snapshot before any mutation so the gesture commits as
        // one undoable action in mouseUp.
        gestureSnapshot = processor.getPitchCurve();

        const auto pos = e.position;
        const int  hit = hitTestPoint(pos);

        if (e.mods.isPopupMenu())
        {
            if (hit >= 0)
            {
                auto curve = processor.getPitchCurve();
                curve.erase(curve.begin() + hit);
                processor.setPitchCurve(std::move(curve));
                repaint();
            }
            return;
        }

        if (hit >= 0)
        {
            draggingIndex = hit;
            return;
        }

        // Click on empty space — add a point at the cursor and begin
        // dragging it so the motion feels continuous from the click.
        const auto newPoint = fromScreen(pos);
        auto curve = processor.getPitchCurve();
        curve.push_back(newPoint);
        sortByTime(curve);

        draggingIndex = -1;
        float bestDist = 1e30f;
        for (size_t i = 0; i < curve.size(); ++i)
        {
            const float d = toScreen(curve[i]).getDistanceFrom(pos);
            if (d < bestDist)
            {
                bestDist      = d;
                draggingIndex = static_cast<int>(i);
            }
        }

        processor.setPitchCurve(std::move(curve));
        repaint();
    }

    void PitchEnvelopeEditor::mouseDrag(const juce::MouseEvent& e)
    {
        if (draggingIndex < 0)
            return;

        auto curve = processor.getPitchCurve();
        if (draggingIndex >= static_cast<int>(curve.size()))
        {
            draggingIndex = -1;
            return;
        }

        curve[static_cast<size_t>(draggingIndex)] = fromScreen(e.position);
        sortByTime(curve);

        // After the sort the dragged point may be at a new index.
        // Its on-screen position matches the mouse by construction,
        // so finding the nearest point to the mouse recovers it
        // robustly without needing a persistent identity.
        int   newIdx   = -1;
        float bestDist = 1e30f;
        for (size_t i = 0; i < curve.size(); ++i)
        {
            const float d = toScreen(curve[i]).getDistanceFrom(e.position);
            if (d < bestDist)
            {
                bestDist = d;
                newIdx   = static_cast<int>(i);
            }
        }
        draggingIndex = newIdx;

        processor.setPitchCurve(std::move(curve));
        repaint();
    }

    void PitchEnvelopeEditor::mouseUp(const juce::MouseEvent&)
    {
        draggingIndex = -1;

        const auto& current = processor.getPitchCurve();
        if (current != gestureSnapshot)
        {
            processor.getUndoManager().beginNewTransaction("Edit pitch curve");
            processor.getUndoManager().perform(
                new SetPitchCurveAction(processor, this,
                                        std::move(gestureSnapshot),
                                        current));
        }
    }
}

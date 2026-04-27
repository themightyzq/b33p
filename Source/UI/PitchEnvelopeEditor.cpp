#include "PitchEnvelopeEditor.h"

#include "State/UndoableActions.h"

#include <algorithm>

namespace B33p
{
    namespace
    {
        constexpr float kSemitoneRange = 12.0f;
        constexpr float kHitRadius     = 10.0f;
        constexpr float kPlotInset     = 6.0f;
        constexpr float kPointRadius   = 5.0f;
        constexpr float kCornerRadius  = 3.0f;
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
        const auto frame = getLocalBounds().toFloat().reduced(1.0f);

        g.setColour(juce::Colour::fromRGB(20, 20, 20));
        g.fillRoundedRectangle(frame, kCornerRadius);
        g.setColour(juce::Colour::fromRGB(60, 60, 60));
        g.drawRoundedRectangle(frame, kCornerRadius, 1.0f);

        const auto area = plotArea();

        g.setColour(juce::Colour::fromRGB(55, 55, 55));
        g.drawHorizontalLine(static_cast<int>(area.getCentreY()),
                             area.getX(), area.getRight());

        const auto& curve = processor.getPitchCurve();
        if (curve.empty())
        {
            // First-run hint — fades the moment a single point is added.
            g.setColour(juce::Colour::fromRGB(110, 110, 110));
            g.setFont(juce::FontOptions(12.0f));
            g.drawText("Click to add pitch points. Drag to shape, right-click to delete.",
                       area, juce::Justification::centred);
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

        g.setColour(juce::Colour::fromRGB(220, 140, 60));
        g.strokePath(path, juce::PathStrokeType(kCurveStroke));

        for (const auto& p : sorted)
        {
            const auto screen = toScreen(p);
            g.setColour(juce::Colour::fromRGB(250, 180, 100));
            g.fillEllipse(screen.x - kPointRadius, screen.y - kPointRadius,
                          kPointRadius * 2.0f, kPointRadius * 2.0f);
            g.setColour(juce::Colour::fromRGB(30, 30, 30));
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

#include "PatternGrid.h"

#include "Pattern/SnapMath.h"

#include <algorithm>
#include <cmath>

namespace B33p
{
    namespace
    {
        constexpr float  kLaneLabelWidth     = 24.0f;
        constexpr float  kRulerHeight        = 20.0f;
        constexpr float  kOuterInset         = 1.0f;
        constexpr float  kLaneInset          = 3.0f;   // vertical padding inside each lane
        constexpr float  kEventCorner        = 2.0f;
        constexpr float  kResizeEdgeWidth    = 6.0f;
        constexpr double kMinDurationSec     = 0.02;   // 20 ms — tighter than the smallest grid by design
        constexpr double kDefaultDurationSec = 0.1;    // matches the default 100 ms grid
    }

    PatternGrid::PatternGrid(B33pProcessor& processorRef)
        : processor(processorRef)
    {
        setWantsKeyboardFocus(true);
    }

    void PatternGrid::setGridSeconds(double seconds)
    {
        gridSeconds = seconds;
        repaint();
    }

    juce::Rectangle<float> PatternGrid::plotArea() const
    {
        return getLocalBounds().toFloat().reduced(kOuterInset);
    }

    juce::Rectangle<float> PatternGrid::laneArea(int lane) const
    {
        auto frame = plotArea();
        const float laneStripTop = frame.getY() + kRulerHeight;
        const float laneStripHeight = frame.getBottom() - laneStripTop;
        const float rowHeight = laneStripHeight / static_cast<float>(Pattern::kNumLanes);
        return juce::Rectangle<float> {
            frame.getX() + kLaneLabelWidth,
            laneStripTop + rowHeight * static_cast<float>(lane),
            frame.getWidth() - kLaneLabelWidth,
            rowHeight
        };
    }

    float PatternGrid::secondsToX(double seconds) const
    {
        auto frame = plotArea();
        const double length = processor.getPattern().getLengthSeconds();
        const float plotLeft  = frame.getX() + kLaneLabelWidth;
        const float plotWidth = frame.getWidth() - kLaneLabelWidth;
        const double t = std::clamp(seconds / length, 0.0, 1.0);
        return plotLeft + static_cast<float>(t) * plotWidth;
    }

    double PatternGrid::xToSeconds(float x) const
    {
        auto frame = plotArea();
        const double length = processor.getPattern().getLengthSeconds();
        const float plotLeft  = frame.getX() + kLaneLabelWidth;
        const float plotWidth = frame.getWidth() - kLaneLabelWidth;
        if (plotWidth <= 0.0f)
            return 0.0;
        return std::clamp((static_cast<double>(x - plotLeft) / plotWidth) * length,
                          0.0, length);
    }

    int PatternGrid::yToLane(float y) const
    {
        auto frame = plotArea();
        const float laneStripTop = frame.getY() + kRulerHeight;
        if (y < laneStripTop || y > frame.getBottom())
            return -1;
        const float rowHeight = (frame.getBottom() - laneStripTop) / Pattern::kNumLanes;
        const int lane = static_cast<int>((y - laneStripTop) / rowHeight);
        return std::clamp(lane, 0, Pattern::kNumLanes - 1);
    }

    double PatternGrid::snapSeconds(double seconds) const
    {
        return snapToGrid(seconds, gridSeconds);
    }

    juce::Rectangle<float> PatternGrid::eventRect(int lane, const Event& e) const
    {
        auto laneRectF = laneArea(lane).reduced(0.0f, kLaneInset);
        const float x1 = secondsToX(e.startSeconds);
        const float x2 = secondsToX(e.startSeconds + e.durationSeconds);
        return { x1, laneRectF.getY(), std::max(2.0f, x2 - x1), laneRectF.getHeight() };
    }

    PatternGrid::HitResult PatternGrid::hitTestEvent(juce::Point<float> p) const
    {
        HitResult none;
        const int lane = yToLane(p.y);
        if (lane < 0)
            return none;

        const auto& events = processor.getPattern().getEvents(lane);
        // Walk newest-to-oldest so later-drawn events receive clicks first.
        for (std::size_t i = events.size(); i-- > 0;)
        {
            const auto rect = eventRect(lane, events[i]);
            if (! rect.contains(p))
                continue;

            HitResult r;
            r.lane  = lane;
            r.index = i;
            r.kind  = (p.x >= rect.getRight() - kResizeEdgeWidth)
                          ? HitResult::Kind::RightEdge
                          : HitResult::Kind::Body;
            return r;
        }
        return none;
    }

    void PatternGrid::paint(juce::Graphics& g)
    {
        auto frame = plotArea();

        g.setColour(juce::Colour::fromRGB(20, 20, 20));
        g.fillRoundedRectangle(frame, 3.0f);
        g.setColour(juce::Colour::fromRGB(60, 60, 60));
        g.drawRoundedRectangle(frame, 3.0f, 1.0f);

        const auto& pattern = processor.getPattern();
        const double length = pattern.getLengthSeconds();

        // Grid time lines (skipped entirely when snap is off so the
        // canvas isn't cluttered).
        if (gridSeconds > 0.0)
        {
            g.setColour(juce::Colour::fromRGB(40, 40, 40));
            for (double t = 0.0; t <= length + 1e-6; t += gridSeconds)
            {
                const float x = secondsToX(t);
                g.drawVerticalLine(static_cast<int>(std::round(x)),
                                   frame.getY() + kRulerHeight,
                                   frame.getBottom());
            }
        }

        // Lane separators
        g.setColour(juce::Colour::fromRGB(55, 55, 55));
        for (int lane = 1; lane < Pattern::kNumLanes; ++lane)
        {
            auto r = laneArea(lane);
            g.drawHorizontalLine(static_cast<int>(r.getY()),
                                 frame.getX() + kLaneLabelWidth,
                                 frame.getRight());
        }
        g.drawHorizontalLine(static_cast<int>(frame.getY() + kRulerHeight),
                             frame.getX() + kLaneLabelWidth,
                             frame.getRight());

        // Ruler second labels
        g.setColour(juce::Colour::fromRGB(170, 170, 170));
        g.setFont(juce::FontOptions(10.0f));
        const int lastSec = static_cast<int>(std::floor(length));
        for (int s = 0; s <= lastSec; ++s)
        {
            const float x = secondsToX(static_cast<double>(s));
            g.drawText(juce::String(s) + "s",
                       juce::Rectangle<float>(x + 2.0f,
                                              frame.getY() + 2.0f,
                                              30.0f,
                                              kRulerHeight - 4.0f),
                       juce::Justification::centredLeft);
        }

        // Lane labels
        g.setColour(juce::Colour::fromRGB(150, 150, 150));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
        {
            auto r = laneArea(lane);
            g.drawText(juce::String(lane + 1),
                       juce::Rectangle<float>(frame.getX(),
                                              r.getY(),
                                              kLaneLabelWidth,
                                              r.getHeight()),
                       juce::Justification::centred);
        }

        // Events
        for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
        {
            const auto& events = pattern.getEvents(lane);
            for (std::size_t i = 0; i < events.size(); ++i)
            {
                // Events past the new pattern length still exist in
                // the data — they just don't draw or play. Shrinking
                // the pattern is non-destructive.
                if (events[i].startSeconds >= length)
                    continue;

                const auto rect = eventRect(lane, events[i]);
                const bool isSelected = (selection.valid()
                                         && selection.lane  == lane
                                         && selection.index == i);

                g.setColour(isSelected
                                ? juce::Colour::fromRGB(120, 200, 255)
                                : juce::Colour::fromRGB(80, 150, 220));
                g.fillRoundedRectangle(rect, kEventCorner);

                g.setColour(isSelected
                                ? juce::Colour::fromRGB(230, 240, 255)
                                : juce::Colour::fromRGB(30, 30, 30));
                g.drawRoundedRectangle(rect, kEventCorner, 1.0f);
            }
        }

        // Playhead — only drawn while the processor is playing.
        if (processor.isPlaying())
        {
            const float x = secondsToX(processor.getPlayheadSeconds());
            g.setColour(juce::Colour::fromRGB(255, 165, 60));
            g.drawLine(x, frame.getY() + kRulerHeight,
                       x, frame.getBottom(),
                       1.5f);
        }
    }

    void PatternGrid::mouseDown(const juce::MouseEvent& e)
    {
        grabKeyboardFocus();

        const auto hit = hitTestEvent(e.position);

        if (hit.kind == HitResult::Kind::None)
        {
            const int lane = yToLane(e.position.y);
            if (lane < 0)
                return;

            const double snappedStart = std::max(0.0, snapSeconds(xToSeconds(e.position.x)));
            const double length = processor.getPattern().getLengthSeconds();
            const double duration = std::min(kDefaultDurationSec,
                                             std::max(kMinDurationSec, length - snappedStart));

            Event newEvent { snappedStart, duration, 0.0f };
            processor.getPattern().addEvent(lane, newEvent);
            processor.markDirty();

            selection.lane  = lane;
            selection.index = processor.getPattern().getEvents(lane).size() - 1;

            dragMode          = DragMode::Move;
            dragStartSeconds  = xToSeconds(e.position.x);
            dragOriginalEvent = newEvent;

            repaint();
            return;
        }

        selection.lane  = hit.lane;
        selection.index = hit.index;

        dragMode          = (hit.kind == HitResult::Kind::RightEdge)
                                ? DragMode::Resize
                                : DragMode::Move;
        dragStartSeconds  = xToSeconds(e.position.x);
        dragOriginalEvent = processor.getPattern().getEvents(hit.lane)[hit.index];

        repaint();
    }

    void PatternGrid::mouseDrag(const juce::MouseEvent& e)
    {
        if (dragMode == DragMode::None || ! selection.valid())
            return;

        auto&       pattern = processor.getPattern();
        const double length = pattern.getLengthSeconds();

        Event current = dragOriginalEvent;

        if (dragMode == DragMode::Move)
        {
            const double deltaSec = xToSeconds(e.position.x) - dragStartSeconds;
            current.startSeconds  = snapSeconds(dragOriginalEvent.startSeconds + deltaSec);
            current.startSeconds  = std::clamp(current.startSeconds,
                                               0.0,
                                               std::max(0.0, length - current.durationSeconds));
        }
        else if (dragMode == DragMode::Resize)
        {
            const double cursorSec = xToSeconds(e.position.x);
            double newDuration = snapSeconds(cursorSec - current.startSeconds);
            newDuration        = std::max(kMinDurationSec, newDuration);
            newDuration        = std::min(newDuration, length - current.startSeconds);
            current.durationSeconds = newDuration;
        }

        pattern.updateEvent(selection.lane, selection.index, current);
        processor.markDirty();
        repaint();
    }

    void PatternGrid::mouseUp(const juce::MouseEvent&)
    {
        dragMode = DragMode::None;
    }

    bool PatternGrid::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
        {
            if (selection.valid())
            {
                processor.getPattern().removeEvent(selection.lane, selection.index);
                processor.markDirty();
                selection = {};
                repaint();
                return true;
            }
        }
        return false;
    }
}

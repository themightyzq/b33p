#include "PatternGrid.h"

#include "Pattern/SnapMath.h"
#include "State/UndoableActions.h"

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

    void PatternGrid::setSelection(const Selection& newSelection)
    {
        const bool changed = (newSelection.lane  != selection.lane
                            || newSelection.index != selection.index);
        selection = newSelection;
        if (changed && onSelectionChanged)
            onSelectionChanged();
    }

    void PatternGrid::clearSelection()
    {
        if (selection.valid())
        {
            setSelection({});
            repaint();
        }
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

        // Velocity drives the visible height: a v=1.0 event fills
        // the whole lane row, v=0.5 is half height. A small floor
        // keeps even silent (v=0) events visible enough to grab.
        const float v        = juce::jlimit(0.0f, 1.0f, e.velocity);
        const float fullH    = laneRectF.getHeight();
        const float minH     = std::min(fullH, 8.0f);
        const float visibleH = std::max(minH, fullH * v);
        const float yCentre  = laneRectF.getCentreY();

        return { x1,
                 yCentre - visibleH * 0.5f,
                 std::max(2.0f, x2 - x1),
                 visibleH };
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
            if      (p.x <= rect.getX()     + kResizeEdgeWidth) r.kind = HitResult::Kind::LeftEdge;
            else if (p.x >= rect.getRight() - kResizeEdgeWidth) r.kind = HitResult::Kind::RightEdge;
            else                                                r.kind = HitResult::Kind::Body;
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

        // First-run hint — drawn only when every lane is empty so it
        // disappears the moment the user adds their first beep.
        bool anyEvents = false;
        for (int lane = 0; lane < Pattern::kNumLanes && ! anyEvents; ++lane)
            anyEvents = ! pattern.getEvents(lane).empty();

        if (! anyEvents)
        {
            const auto hintArea = juce::Rectangle<float>(
                frame.getX() + kLaneLabelWidth,
                frame.getY() + kRulerHeight,
                frame.getWidth() - kLaneLabelWidth,
                frame.getBottom() - (frame.getY() + kRulerHeight));

            g.setColour(juce::Colour::fromRGB(110, 110, 110));
            g.setFont(juce::FontOptions(12.0f));
            g.drawText("Drag in a lane to draw a beep, or double-click for default size. "
                       "Drag a beep to move it (vertically to switch lanes), drag its edges to resize.",
                       hintArea, juce::Justification::centred);
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
                const bool isHover    = (hover.valid()
                                         && hover.lane  == lane
                                         && hover.index == i);

                // Fill: selected > hover > idle. The hover lift is
                // small (10..15 RGB units) — enough to read as
                // "this is the click target" without competing
                // with the selection accent.
                juce::Colour fill { 80, 150, 220 };
                if (isSelected)   fill = { 120, 200, 255 };
                else if (isHover) fill = {  95, 170, 235 };
                g.setColour(fill);
                g.fillRoundedRectangle(rect, kEventCorner);

                g.setColour(isSelected
                                ? juce::Colour::fromRGB(230, 240, 255)
                                : juce::Colour::fromRGB(30, 30, 30));
                g.drawRoundedRectangle(rect, kEventCorner,
                                        isSelected ? 1.6f : 1.0f);

                // Resize-handle dots on the selected event so the
                // edges read as grabbable handles, not just an outline.
                if (isSelected)
                {
                    const float r  = 2.5f;
                    const float cy = rect.getCentreY();
                    g.setColour(juce::Colour::fromRGB(230, 240, 255));
                    g.fillEllipse(rect.getX()      - r, cy - r, r * 2.0f, r * 2.0f);
                    g.fillEllipse(rect.getRight()  - r, cy - r, r * 2.0f, r * 2.0f);
                }
            }
        }

        // Snap-target preview — drawn during an active drag to show
        // where the manipulated edge will land on release.
        if (snapPreviewSeconds >= 0.0)
        {
            const float x = secondsToX(snapPreviewSeconds);
            g.setColour(juce::Colour::fromRGB(230, 240, 255).withAlpha(0.55f));
            g.drawLine(x, frame.getY() + kRulerHeight,
                       x, frame.getBottom(),
                       1.0f);
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

        // Right-click on an event opens a context menu (Delete /
        // Duplicate). Right-click on empty grid is a no-op.
        if (e.mods.isPopupMenu())
        {
            const auto hit = hitTestEvent(e.position);
            if (hit.kind == HitResult::Kind::None)
                return;

            setSelection({ hit.lane, hit.index });
            repaint();

            juce::PopupMenu menu;
            menu.addItem(1, "Delete");
            menu.addItem(2, "Duplicate");
            const int laneClicked  = hit.lane;
            const std::size_t idxClicked = hit.index;
            menu.showMenuAsync(juce::PopupMenu::Options{},
                [this, laneClicked, idxClicked](int result)
                {
                    auto& pattern = processor.getPattern();
                    if (idxClicked >= pattern.getEvents(laneClicked).size())
                        return;

                    Pattern before = pattern;
                    if (result == 1)
                    {
                        pattern.removeEvent(laneClicked, idxClicked);
                        setSelection({});
                    }
                    else if (result == 2)
                    {
                        Event copy = pattern.getEvents(laneClicked)[idxClicked];
                        // Place the duplicate immediately after the original.
                        copy.startSeconds = std::min(
                            pattern.getLengthSeconds() - copy.durationSeconds,
                            copy.startSeconds + copy.durationSeconds);
                        pattern.addEvent(laneClicked, copy);
                        setSelection({ laneClicked,
                                       pattern.getEvents(laneClicked).size() - 1 });
                    }
                    else
                    {
                        return; // dismissed
                    }

                    processor.markDirty();
                    repaint();

                    processor.getUndoManager().beginNewTransaction(
                        result == 1 ? "Delete event" : "Duplicate event");
                    processor.getUndoManager().perform(
                        new SetPatternAction(processor, this,
                                             std::move(before),
                                             pattern));
                });
            return;
        }

        // Snapshot the pattern before any mutation so mouseUp can
        // wrap the whole gesture in a single undoable action.
        gestureSnapshot = processor.getPattern();
        mouseDownPos    = e.position;

        const auto hit = hitTestEvent(e.position);

        if (hit.kind == HitResult::Kind::None)
        {
            const int lane = yToLane(e.position.y);
            if (lane < 0)
                return;

            // Defer creation: this might be a click-to-deselect or a
            // drag-to-create-with-size. mouseDrag promotes to a real
            // event once the cursor moves past the drag threshold;
            // mouseUp without drag deselects.
            pendingCreateLane = lane;
            dragStartSeconds  = xToSeconds(e.position.x);
            dragMode          = DragMode::PendingCreate;
            return;
        }

        setSelection({ hit.lane, hit.index });

        switch (hit.kind)
        {
            case HitResult::Kind::LeftEdge:  dragMode = DragMode::ResizeLeft;  break;
            case HitResult::Kind::RightEdge: dragMode = DragMode::ResizeRight; break;
            case HitResult::Kind::Body:
            case HitResult::Kind::None:
            default:                         dragMode = DragMode::Move;        break;
        }
        dragStartSeconds  = xToSeconds(e.position.x);
        dragOriginalEvent = processor.getPattern().getEvents(hit.lane)[hit.index];

        repaint();
    }

    void PatternGrid::mouseDrag(const juce::MouseEvent& e)
    {
        if (dragMode == DragMode::None)
            return;

        auto&       pattern = processor.getPattern();
        const double length = pattern.getLengthSeconds();

        // Promote a pending click into a real event the first time
        // the cursor moves past the standard drag threshold.
        if (dragMode == DragMode::PendingCreate)
        {
            constexpr float kDragThreshold = 4.0f;
            if (e.position.getDistanceFrom(mouseDownPos) < kDragThreshold)
                return;

            const double snappedStart = std::max(0.0, snapSeconds(dragStartSeconds));
            const double cursorSec    = xToSeconds(e.position.x);
            double duration           = snapSeconds(cursorSec - snappedStart);
            duration                  = std::max(kMinDurationSec, duration);
            duration                  = std::min(duration, length - snappedStart);

            Event newEvent { snappedStart, duration, 0.0f, 1.0f };
            pattern.addEvent(pendingCreateLane, newEvent);
            processor.markDirty();

            setSelection({ pendingCreateLane,
                           pattern.getEvents(pendingCreateLane).size() - 1 });

            dragOriginalEvent = newEvent;
            dragMode          = DragMode::ResizeRight;   // continue sizing
            pendingCreateLane = -1;
            // Fall through to the ResizeRight branch below to apply
            // the current cursor position as the new right edge.
        }

        if (! selection.valid())
            return;

        Event current = dragOriginalEvent;

        if (dragMode == DragMode::Move)
        {
            const double deltaSec = xToSeconds(e.position.x) - dragStartSeconds;
            current.startSeconds  = snapSeconds(dragOriginalEvent.startSeconds + deltaSec);
            current.startSeconds  = std::clamp(current.startSeconds,
                                               0.0,
                                               std::max(0.0, length - current.durationSeconds));
            snapPreviewSeconds = current.startSeconds;

            // Vertical drag retargets the lane. Snap to lane row so
            // the event jumps cleanly between lanes rather than
            // sitting visually mid-boundary.
            const int targetLane = yToLane(e.position.y);
            if (targetLane >= 0 && targetLane != selection.lane)
            {
                pattern.removeEvent(selection.lane, selection.index);
                pattern.addEvent(targetLane, current);
                processor.markDirty();
                setSelection({ targetLane,
                               pattern.getEvents(targetLane).size() - 1 });
                repaint();
                if (onSelectionChanged) onSelectionChanged();
                return;
            }
        }
        else if (dragMode == DragMode::ResizeRight)
        {
            const double cursorSec = xToSeconds(e.position.x);
            double newDuration = snapSeconds(cursorSec - current.startSeconds);
            newDuration        = std::max(kMinDurationSec, newDuration);
            newDuration        = std::min(newDuration, length - current.startSeconds);
            current.durationSeconds = newDuration;
            snapPreviewSeconds = current.startSeconds + current.durationSeconds;
        }
        else if (dragMode == DragMode::ResizeLeft)
        {
            // End point stays anchored; start follows the cursor and
            // duration shrinks/grows to compensate.
            const double endSec   = dragOriginalEvent.startSeconds
                                   + dragOriginalEvent.durationSeconds;
            double newStart       = snapSeconds(xToSeconds(e.position.x));
            newStart              = std::max(0.0,
                std::min(newStart, endSec - kMinDurationSec));
            current.startSeconds    = newStart;
            current.durationSeconds = endSec - newStart;
            snapPreviewSeconds      = newStart;
        }

        pattern.updateEvent(selection.lane, selection.index, current);
        processor.markDirty();
        repaint();

        // Inspector below tracks the live drag values, not just the
        // mouseUp end state. Cheap to fire 30+ times per drag.
        if (onSelectionChanged)
            onSelectionChanged();
    }

    void PatternGrid::mouseMove(const juce::MouseEvent& e)
    {
        const auto hit = hitTestEvent(e.position);
        Selection newHover;
        if (hit.kind != HitResult::Kind::None)
            newHover = { hit.lane, hit.index };

        if (newHover.lane != hover.lane || newHover.index != hover.index)
        {
            hover = newHover;
            repaint();
        }

        // Cursor affordance: edge = resize, body = drag, lane row =
        // crosshair (will create / select), elsewhere = normal.
        juce::MouseCursor cur { juce::MouseCursor::NormalCursor };
        if      (hit.kind == HitResult::Kind::LeftEdge
              || hit.kind == HitResult::Kind::RightEdge) cur = juce::MouseCursor::LeftRightResizeCursor;
        else if (hit.kind == HitResult::Kind::Body)      cur = juce::MouseCursor::DraggingHandCursor;
        else if (yToLane(e.position.y) >= 0)             cur = juce::MouseCursor::CrosshairCursor;
        setMouseCursor(cur);
    }

    void PatternGrid::mouseExit(const juce::MouseEvent&)
    {
        if (hover.valid())
        {
            hover = {};
            repaint();
        }
    }

    void PatternGrid::mouseDoubleClick(const juce::MouseEvent& e)
    {
        // Double-click only matters on empty grid — over an event it
        // falls through to the single-click selection behaviour from
        // mouseDown. Creates with the default duration so users have
        // a discoverable alternative to drag-to-create.
        const auto hit = hitTestEvent(e.position);
        if (hit.kind != HitResult::Kind::None)
            return;

        const int lane = yToLane(e.position.y);
        if (lane < 0)
            return;

        Pattern before = processor.getPattern();
        const double snappedStart = std::max(0.0, snapSeconds(xToSeconds(e.position.x)));
        const double length = before.getLengthSeconds();
        const double duration = std::min(kDefaultDurationSec,
                                         std::max(kMinDurationSec, length - snappedStart));

        Event newEvent { snappedStart, duration, 0.0f, 1.0f };
        processor.getPattern().addEvent(lane, newEvent);
        processor.markDirty();
        setSelection({ lane,
                       processor.getPattern().getEvents(lane).size() - 1 });
        repaint();

        processor.getUndoManager().beginNewTransaction("Create event");
        processor.getUndoManager().perform(
            new SetPatternAction(processor, this,
                                 std::move(before),
                                 processor.getPattern()));
    }

    void PatternGrid::mouseUp(const juce::MouseEvent&)
    {
        // PendingCreate that never crossed the drag threshold = a
        // bare click on empty grid. Treat it as "deselect".
        if (dragMode == DragMode::PendingCreate)
        {
            pendingCreateLane = -1;
            dragMode          = DragMode::None;
            if (selection.valid())
                clearSelection();
            return;
        }

        dragMode = DragMode::None;

        if (snapPreviewSeconds >= 0.0)
        {
            snapPreviewSeconds = -1.0;
            repaint();
        }

        // Commit the gesture as a single undoable transaction iff
        // it actually changed the pattern. Right-clicks above the
        // lane area produce no diff and skip cleanly.
        const auto& current = processor.getPattern();
        if (current != gestureSnapshot)
        {
            processor.getUndoManager().beginNewTransaction("Edit pattern");
            processor.getUndoManager().perform(
                new SetPatternAction(processor, this,
                                     std::move(gestureSnapshot),
                                     current));
        }
    }

    bool PatternGrid::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
        {
            if (selection.valid())
            {
                Pattern before = processor.getPattern();
                processor.getPattern().removeEvent(selection.lane, selection.index);
                processor.markDirty();
                setSelection({});
                repaint();

                processor.getUndoManager().beginNewTransaction("Delete event");
                processor.getUndoManager().perform(
                    new SetPatternAction(processor, this,
                                         std::move(before),
                                         processor.getPattern()));
                return true;
            }
        }

        if (key == juce::KeyPress::escapeKey)
        {
            if (selection.valid())
            {
                clearSelection();
                return true;
            }
            return false;
        }

        // Arrow keys nudge the selected event. Step = one grid
        // tick; shift+arrow steps by ten. Falls back to 10 ms when
        // the grid is "Off".
        const bool isLeft  = (key == juce::KeyPress::leftKey);
        const bool isRight = (key == juce::KeyPress::rightKey);
        if ((isLeft || isRight) && selection.valid())
        {
            const double step    = (gridSeconds > 0.0 ? gridSeconds : 0.01);
            const int    mult    = key.getModifiers().isShiftDown() ? 10 : 1;
            const double delta   = (isRight ? 1.0 : -1.0) * step * mult;

            auto&  pattern = processor.getPattern();
            if (selection.index >= pattern.getEvents(selection.lane).size())
                return true;

            Pattern before = pattern;
            Event   ev = pattern.getEvents(selection.lane)[selection.index];
            const double maxStart = std::max(0.0,
                pattern.getLengthSeconds() - ev.durationSeconds);
            ev.startSeconds = std::clamp(ev.startSeconds + delta, 0.0, maxStart);

            if (ev == pattern.getEvents(selection.lane)[selection.index])
                return true;   // hit a clamp; nothing to do

            pattern.updateEvent(selection.lane, selection.index, ev);
            processor.markDirty();
            repaint();

            if (onSelectionChanged)
                onSelectionChanged();

            processor.getUndoManager().beginNewTransaction("Nudge event");
            processor.getUndoManager().perform(
                new SetPatternAction(processor, this,
                                     std::move(before),
                                     pattern));
            return true;
        }

        return false;
    }
}

#include "PatternGrid.h"

#include "Pattern/SnapMath.h"
#include "State/UndoableActions.h"

#include <algorithm>
#include <cmath>

namespace B33p
{
    namespace
    {
        constexpr float  kLaneLabelWidth     = 100.0f; // wider to fit name + mute + solo
        constexpr float  kLaneLabelInset     = 4.0f;
        constexpr float  kMuteButtonWidth    = 22.0f;
        constexpr float  kSoloButtonWidth    = 22.0f;
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

        for (int i = 0; i < Pattern::kNumLanes; ++i)
        {
            auto& nameLabel = laneNameLabels[static_cast<size_t>(i)];
            auto& muteBtn   = muteButtons    [static_cast<size_t>(i)];

            nameLabel.setEditable(false, true, false);   // double-click to edit
            nameLabel.setJustificationType(juce::Justification::centredLeft);
            nameLabel.setFont(juce::FontOptions(11.0f));
            nameLabel.setColour(juce::Label::textColourId,
                                  juce::Colour::fromRGB(190, 190, 190));
            nameLabel.setColour(juce::Label::backgroundWhenEditingColourId,
                                  juce::Colour::fromRGB(28, 28, 28));
            nameLabel.setTooltip("Double-click to rename");
            nameLabel.onTextChange = [this, i]
            {
                processor.setSelectedLane(i);
                const auto displayed   = laneNameLabels[static_cast<size_t>(i)].getText().trim();
                const auto defaultText = juce::String(i + 1);
                const juce::String stored = (displayed == defaultText)
                                                ? juce::String{}
                                                : displayed;
                if (processor.getPattern().getLaneName(i) == stored)
                    return;

                Pattern before = processor.getPattern();
                processor.getPattern().setLaneName(i, stored);
                processor.markDirty();
                refreshLaneMetaFromPattern();   // re-show fallback if stored is empty

                processor.getUndoManager().beginNewTransaction("Rename lane");
                processor.getUndoManager().perform(
                    new SetPatternAction(processor, this,
                                         std::move(before),
                                         processor.getPattern()));
            };
            addAndMakeVisible(nameLabel);

            muteBtn.setButtonText("M");
            muteBtn.setClickingTogglesState(true);
            muteBtn.setTooltip("Mute lane");
            muteBtn.setColour(juce::TextButton::buttonOnColourId,
                                juce::Colour::fromRGB(190, 60, 60));
            muteBtn.onClick = [this, i]
            {
                processor.setSelectedLane(i);
                const bool wantMuted = muteButtons[static_cast<size_t>(i)].getToggleState();
                if (processor.getPattern().isLaneMuted(i) == wantMuted)
                    return;

                Pattern before = processor.getPattern();
                processor.getPattern().setLaneMuted(i, wantMuted);
                processor.markDirty();

                processor.getUndoManager().beginNewTransaction("Toggle lane mute");
                processor.getUndoManager().perform(
                    new SetPatternAction(processor, this,
                                         std::move(before),
                                         processor.getPattern()));
            };
            addAndMakeVisible(muteBtn);

            auto& soloBtn = soloButtons[static_cast<size_t>(i)];
            soloBtn.setButtonText("S");
            soloBtn.setClickingTogglesState(true);
            soloBtn.setTooltip("Solo lane (only soloed lanes play)");
            soloBtn.setColour(juce::TextButton::buttonOnColourId,
                                juce::Colour::fromRGB(220, 200, 60));
            soloBtn.onClick = [this, i]
            {
                processor.setSelectedLane(i);
                const bool wantSoloed = soloButtons[static_cast<size_t>(i)].getToggleState();
                if (processor.getPattern().isLaneSoloed(i) == wantSoloed)
                    return;

                Pattern before = processor.getPattern();
                processor.getPattern().setLaneSoloed(i, wantSoloed);
                processor.markDirty();

                processor.getUndoManager().beginNewTransaction("Toggle lane solo");
                processor.getUndoManager().perform(
                    new SetPatternAction(processor, this,
                                         std::move(before),
                                         processor.getPattern()));
            };
            addAndMakeVisible(soloBtn);
        }

        refreshLaneMetaFromPattern();
    }

    void PatternGrid::resized()
    {
        // Lane label children sit in the kLaneLabelWidth strip on
        // the left, one row per lane. Each row is name + mute side
        // by side, vertically centred in the lane row.
        for (int i = 0; i < Pattern::kNumLanes; ++i)
        {
            auto laneRow = laneArea(i).toNearestInt();
            auto strip   = juce::Rectangle<int> { 0, laneRow.getY(),
                                                   static_cast<int>(kLaneLabelWidth),
                                                   laneRow.getHeight() }
                              .reduced(static_cast<int>(kLaneLabelInset));

            const int muteW = static_cast<int>(kMuteButtonWidth);
            const int soloW = static_cast<int>(kSoloButtonWidth);
            soloButtons   [static_cast<size_t>(i)].setBounds(strip.removeFromRight(soloW));
            strip.removeFromRight(2);
            muteButtons   [static_cast<size_t>(i)].setBounds(strip.removeFromRight(muteW));
            strip.removeFromRight(2);
            laneNameLabels[static_cast<size_t>(i)].setBounds(strip);
        }
    }

    void PatternGrid::refreshLaneMetaFromPattern()
    {
        for (int i = 0; i < Pattern::kNumLanes; ++i)
        {
            const auto& storedName = processor.getPattern().getLaneName(i);
            const auto displayed   = storedName.isEmpty()
                                         ? juce::String(i + 1)
                                         : storedName;
            laneNameLabels[static_cast<size_t>(i)].setText(
                displayed, juce::dontSendNotification);
            muteButtons[static_cast<size_t>(i)].setToggleState(
                processor.getPattern().isLaneMuted(i),
                juce::dontSendNotification);
            soloButtons[static_cast<size_t>(i)].setToggleState(
                processor.getPattern().isLaneSoloed(i),
                juce::dontSendNotification);
        }
    }

    void PatternGrid::setGridSeconds(double seconds)
    {
        gridSeconds = seconds;
        repaint();
    }

    PatternGrid::Selection PatternGrid::getPrimarySelection() const
    {
        if (selection.empty())
            return {};
        return selection.back();
    }

    bool PatternGrid::isInSelection(const Selection& s) const
    {
        for (const auto& existing : selection)
            if (existing == s) return true;
        return false;
    }

    void PatternGrid::notifySelectionChanged()
    {
        if (onSelectionChanged)
            onSelectionChanged();
    }

    void PatternGrid::selectOnly(const Selection& s)
    {
        if (selection.size() == 1 && selection.back() == s)
            return;
        selection.clear();
        if (s.valid())
            selection.push_back(s);
        notifySelectionChanged();
    }

    void PatternGrid::addToSelection(const Selection& s)
    {
        if (! s.valid() || isInSelection(s))
            return;
        selection.push_back(s);
        notifySelectionChanged();
    }

    void PatternGrid::toggleInSelection(const Selection& s)
    {
        if (! s.valid())
            return;
        for (auto it = selection.begin(); it != selection.end(); ++it)
            if (*it == s)
            {
                selection.erase(it);
                notifySelectionChanged();
                return;
            }
        selection.push_back(s);
        notifySelectionChanged();
    }

    void PatternGrid::clearSelection()
    {
        if (selection.empty())
            return;
        selection.clear();
        notifySelectionChanged();
        repaint();
    }

    void PatternGrid::selectAll()
    {
        selection.clear();
        const auto& pattern = processor.getPattern();
        for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
        {
            const auto& events = pattern.getEvents(lane);
            for (std::size_t i = 0; i < events.size(); ++i)
                selection.push_back({ lane, i });
        }
        notifySelectionChanged();
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

        // Selected-lane tint — subtle background so the user sees
        // which lane the editor sections are currently targeting.
        {
            const int sel = processor.getSelectedLane();
            if (sel >= 0 && sel < Pattern::kNumLanes)
            {
                auto sl = laneArea(sel);
                g.setColour(juce::Colour::fromRGB(120, 200, 255).withAlpha(0.07f));
                g.fillRect(sl);
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

        // Ruler row — slightly raised background + tick marks at
        // every second so the user can SEE it as a clickable strip
        // (clicking it parks the playhead).
        {
            auto rulerRow = juce::Rectangle<float>(
                frame.getX() + kLaneLabelWidth,
                frame.getY(),
                frame.getWidth() - kLaneLabelWidth,
                kRulerHeight);
            g.setColour(juce::Colour::fromRGB(40, 40, 40));
            g.fillRect(rulerRow);

            // Tick marks across the bottom of the ruler.
            g.setColour(juce::Colour::fromRGB(100, 100, 100));
            const int lastSec = static_cast<int>(std::floor(length));
            for (int s = 0; s <= lastSec; ++s)
            {
                const float x = secondsToX(static_cast<double>(s));
                g.drawVerticalLine(static_cast<int>(std::round(x)),
                                    rulerRow.getBottom() - 4.0f,
                                    rulerRow.getBottom());
            }

            // Second labels.
            g.setColour(juce::Colour::fromRGB(190, 190, 190));
            g.setFont(juce::FontOptions(10.0f));
            for (int s = 0; s <= lastSec; ++s)
            {
                const float x = secondsToX(static_cast<double>(s));
                g.drawText(juce::String(s) + "s",
                           juce::Rectangle<float>(x + 2.0f,
                                                  rulerRow.getY() + 1.0f,
                                                  30.0f,
                                                  kRulerHeight - 6.0f),
                           juce::Justification::centredLeft);
            }
        }

        // Lane labels are juce::Label children — see resized() and the
        // ctor's per-lane setup. Nothing to paint here.

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
                const bool isSelected = isInSelection({ lane, i });
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

        // Playhead — orange while playing, dimmer grey when parked
        // (so the user can see where Cmd+V will paste). Hidden only
        // when stopped at exactly 0, which is the no-op resting state.
        {
            const double headSec = processor.getPlayheadSeconds();
            if (processor.isPlaying() || headSec > 0.0)
            {
                const float x = secondsToX(headSec);
                g.setColour(processor.isPlaying()
                                ? juce::Colour::fromRGB(255, 165, 60)
                                : juce::Colour::fromRGB(140, 140, 140));
                g.drawLine(x, frame.getY() + kRulerHeight,
                           x, frame.getBottom(),
                           1.5f);
            }
        }
    }

    void PatternGrid::mouseDown(const juce::MouseEvent& e)
    {
        grabKeyboardFocus();

        // Click on the ruler row parks the playhead. Cmd+V then
        // pastes the clipboard there. Pre-empts every other gesture
        // because the ruler isn't a lane, so there's nothing else
        // meaningful for it to do.
        {
            const auto frame = plotArea();
            const float laneStripStartX = frame.getX() + kLaneLabelWidth;
            const bool inRuler = (e.position.y >= frame.getY()
                                && e.position.y <  frame.getY() + kRulerHeight
                                && e.position.x >= laneStripStartX);
            if (inRuler)
            {
                processor.setPlayheadSeconds(xToSeconds(e.position.x));
                repaint();
                return;
            }
        }

        // Right-click on an event opens a context menu (Delete /
        // Duplicate). Right-click on empty grid is a no-op.
        if (e.mods.isPopupMenu())
        {
            const auto hit = hitTestEvent(e.position);
            if (hit.kind == HitResult::Kind::None)
                return;

            // Single-select the right-clicked event so the menu has
            // an unambiguous target.
            selectOnly({ hit.lane, hit.index });
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
                        clearSelection();
                    }
                    else if (result == 2)
                    {
                        Event copy = pattern.getEvents(laneClicked)[idxClicked];
                        // Place the duplicate immediately after the original.
                        copy.startSeconds = std::min(
                            pattern.getLengthSeconds() - copy.durationSeconds,
                            copy.startSeconds + copy.durationSeconds);
                        pattern.addEvent(laneClicked, copy);
                        selectOnly({ laneClicked,
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

            // Clicking anywhere in a lane row also targets that lane
            // for the editor sections.
            processor.setSelectedLane(lane);

            // Defer creation: this might be a click-to-deselect or a
            // drag-to-create-with-size. mouseDrag promotes to a real
            // event once the cursor moves past the drag threshold;
            // mouseUp without drag deselects.
            pendingCreateLane = lane;
            dragStartSeconds  = xToSeconds(e.position.x);
            dragMode          = DragMode::PendingCreate;
            return;
        }

        // Clicking on an existing event also targets its lane.
        processor.setSelectedLane(hit.lane);

        const Selection clicked { hit.lane, hit.index };

        // Selection rules:
        //   - shift-click: toggle clicked event in/out of selection
        //   - click on already-selected event: keep selection,
        //     promote clicked to primary
        //   - plain click on unselected event: replace selection
        if (e.mods.isShiftDown())
        {
            toggleInSelection(clicked);
        }
        else if (isInSelection(clicked))
        {
            // Promote to primary by re-adding (vector keeps last
            // occurrence semantics; remove then push to back).
            for (auto it = selection.begin(); it != selection.end(); ++it)
                if (*it == clicked) { selection.erase(it); break; }
            selection.push_back(clicked);
            notifySelectionChanged();
        }
        else
        {
            selectOnly(clicked);
        }

        // If shift toggled the event OUT of the selection, there is
        // nothing to drag — bail.
        if (! isInSelection(clicked))
        {
            dragMode = DragMode::None;
            repaint();
            return;
        }

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

        // For Move, capture every selected event's original position
        // so mouseDrag can shift them all by the same delta. Resize
        // gestures still operate on the primary only.
        dragSubjects.clear();
        if (dragMode == DragMode::Move)
        {
            for (const auto& sel : selection)
            {
                const auto& events = processor.getPattern().getEvents(sel.lane);
                if (sel.index < events.size())
                    dragSubjects.push_back({ sel, events[sel.index] });
            }
        }

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

            selectOnly({ pendingCreateLane,
                         pattern.getEvents(pendingCreateLane).size() - 1 });

            dragOriginalEvent = newEvent;
            dragMode          = DragMode::ResizeRight;   // continue sizing
            pendingCreateLane = -1;
            // Fall through to the ResizeRight branch below to apply
            // the current cursor position as the new right edge.
        }

        if (selection.empty())
            return;

        const Selection primary = selection.back();

        if (dragMode == DragMode::Move)
        {
            const double deltaSec = xToSeconds(e.position.x) - dragStartSeconds;

            // Apply the same horizontal delta to every selected
            // event from its original position, snapping each
            // independently and clamping into the pattern.
            for (const auto& subj : dragSubjects)
            {
                Event ev = subj.original;
                ev.startSeconds = snapSeconds(subj.original.startSeconds + deltaSec);
                ev.startSeconds = std::clamp(ev.startSeconds,
                                              0.0,
                                              std::max(0.0, length - ev.durationSeconds));
                if (subj.ref.index < pattern.getEvents(subj.ref.lane).size())
                    pattern.updateEvent(subj.ref.lane, subj.ref.index, ev);
            }
            // Snap-preview line follows the primary's start.
            if (primary.index < pattern.getEvents(primary.lane).size())
                snapPreviewSeconds = pattern.getEvents(primary.lane)[primary.index].startSeconds;

            // Lane retargeting only applies when exactly one event
            // is selected (multi-drag is horizontal-only — ALL
            // moving to one lane would just stack on top of each
            // other).
            if (selection.size() == 1)
            {
                const int targetLane = yToLane(e.position.y);
                if (targetLane >= 0 && targetLane != primary.lane)
                {
                    Event ev = pattern.getEvents(primary.lane)[primary.index];
                    pattern.removeEvent(primary.lane, primary.index);
                    pattern.addEvent(targetLane, ev);
                    processor.markDirty();
                    selectOnly({ targetLane,
                                  pattern.getEvents(targetLane).size() - 1 });
                    // Re-anchor the drag in the new lane.
                    dragSubjects.clear();
                    dragSubjects.push_back({ selection.back(), ev });
                    repaint();
                    notifySelectionChanged();
                    return;
                }
            }
        }
        else if (dragMode == DragMode::ResizeRight)
        {
            Event current = dragOriginalEvent;
            const double cursorSec = xToSeconds(e.position.x);
            double newDuration = snapSeconds(cursorSec - current.startSeconds);
            newDuration        = std::max(kMinDurationSec, newDuration);
            newDuration        = std::min(newDuration, length - current.startSeconds);
            current.durationSeconds = newDuration;
            snapPreviewSeconds = current.startSeconds + current.durationSeconds;
            pattern.updateEvent(primary.lane, primary.index, current);
        }
        else if (dragMode == DragMode::ResizeLeft)
        {
            Event current = dragOriginalEvent;
            const double endSec   = dragOriginalEvent.startSeconds
                                   + dragOriginalEvent.durationSeconds;
            double newStart       = snapSeconds(xToSeconds(e.position.x));
            newStart              = std::max(0.0,
                std::min(newStart, endSec - kMinDurationSec));
            current.startSeconds    = newStart;
            current.durationSeconds = endSec - newStart;
            snapPreviewSeconds      = newStart;
            pattern.updateEvent(primary.lane, primary.index, current);
        }

        processor.markDirty();
        // Push the live drag state into the audio thread immediately
        // instead of waiting for the next 30 Hz snapshot tick — keeps
        // the dragged event audible at its CURRENT position rather
        // than its position 33 ms ago.
        processor.refreshPatternSnapshot();
        repaint();

        // Inspector below tracks the live drag values, not just the
        // mouseUp end state. Cheap to fire 30+ times per drag.
        notifySelectionChanged();
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

        processor.setSelectedLane(lane);

        Pattern before = processor.getPattern();
        const double snappedStart = std::max(0.0, snapSeconds(xToSeconds(e.position.x)));
        const double length = before.getLengthSeconds();
        const double duration = std::min(kDefaultDurationSec,
                                         std::max(kMinDurationSec, length - snappedStart));

        Event newEvent { snappedStart, duration, 0.0f, 1.0f };
        processor.getPattern().addEvent(lane, newEvent);
        processor.markDirty();
        selectOnly({ lane,
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
            if (! selection.empty())
                clearSelection();
            return;
        }

        dragMode = DragMode::None;
        dragSubjects.clear();

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
        const auto& mods = key.getModifiers();
        const bool  cmd  = mods.isCommandDown();

        // Cmd+A — select all events.
        if (cmd && key.getKeyCode() == 'A')
        {
            selectAll();
            return true;
        }

        // Cmd+C — copy the selected events into the internal
        // clipboard with their startSeconds rebased to the earliest
        // selected event's start (so paste preserves their relative
        // timing wherever it's pasted).
        if (cmd && key.getKeyCode() == 'C')
        {
            if (selection.empty()) return true;

            double minStart = std::numeric_limits<double>::infinity();
            for (const auto& sel : selection)
            {
                const auto& evs = processor.getPattern().getEvents(sel.lane);
                if (sel.index < evs.size())
                    minStart = std::min(minStart, evs[sel.index].startSeconds);
            }
            if (! std::isfinite(minStart))
                return true;

            clipboard.clear();
            for (const auto& sel : selection)
            {
                const auto& evs = processor.getPattern().getEvents(sel.lane);
                if (sel.index >= evs.size()) continue;
                Event copy = evs[sel.index];
                copy.startSeconds -= minStart;
                clipboard.push_back({ sel.lane, copy });
            }
            return true;
        }

        // Cmd+V — paste the clipboard at the playhead. Each clipboard
        // item is a (lane, event-with-relative-start); rebase to the
        // playhead's absolute time and add to the corresponding lane.
        if (cmd && key.getKeyCode() == 'V')
        {
            if (clipboard.empty()) return true;

            auto& pattern  = processor.getPattern();
            const double base = processor.getPlayheadSeconds();
            const double len  = pattern.getLengthSeconds();

            Pattern before = pattern;
            std::vector<Selection> newSelection;
            for (const auto& clip : clipboard)
            {
                Event ev = clip.event;
                ev.startSeconds += base;
                if (ev.startSeconds < 0.0 || ev.startSeconds >= len)
                    continue;
                pattern.addEvent(clip.lane, ev);
                newSelection.push_back({ clip.lane,
                                          pattern.getEvents(clip.lane).size() - 1 });
            }
            if (pattern == before)
                return true;
            processor.markDirty();
            selection = std::move(newSelection);
            notifySelectionChanged();
            repaint();

            processor.getUndoManager().beginNewTransaction("Paste events");
            processor.getUndoManager().perform(
                new SetPatternAction(processor, this,
                                     std::move(before),
                                     pattern));
            return true;
        }

        if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
        {
            if (selection.empty()) return false;

            Pattern before = processor.getPattern();

            // Delete from highest (lane, index) downwards so removing
            // an earlier index in the same lane doesn't shift the
            // later indices we still hold in `selection`.
            auto sorted = selection;
            std::sort(sorted.begin(), sorted.end(),
                [](const Selection& a, const Selection& b)
                {
                    if (a.lane != b.lane) return a.lane > b.lane;
                    return a.index > b.index;
                });
            for (const auto& sel : sorted)
                processor.getPattern().removeEvent(sel.lane, sel.index);

            processor.markDirty();
            clearSelection();
            repaint();

            processor.getUndoManager().beginNewTransaction("Delete events");
            processor.getUndoManager().perform(
                new SetPatternAction(processor, this,
                                     std::move(before),
                                     processor.getPattern()));
            return true;
        }

        if (key == juce::KeyPress::escapeKey)
        {
            if (! selection.empty())
            {
                clearSelection();
                return true;
            }
            return false;
        }

        // Arrow keys nudge every selected event. Step = one grid
        // tick; shift+arrow steps by ten. Falls back to 10 ms when
        // the grid is "Off".
        const bool isLeft  = (key == juce::KeyPress::leftKey);
        const bool isRight = (key == juce::KeyPress::rightKey);
        if ((isLeft || isRight) && ! selection.empty())
        {
            const double step    = (gridSeconds > 0.0 ? gridSeconds : 0.01);
            const int    mult    = mods.isShiftDown() ? 10 : 1;
            const double delta   = (isRight ? 1.0 : -1.0) * step * mult;

            auto&  pattern = processor.getPattern();
            Pattern before = pattern;
            bool any = false;
            for (const auto& sel : selection)
            {
                if (sel.index >= pattern.getEvents(sel.lane).size())
                    continue;
                Event ev = pattern.getEvents(sel.lane)[sel.index];
                const double maxStart = std::max(0.0,
                    pattern.getLengthSeconds() - ev.durationSeconds);
                const double newStart = std::clamp(ev.startSeconds + delta, 0.0, maxStart);
                if (! juce::exactlyEqual(newStart, ev.startSeconds))
                {
                    ev.startSeconds = newStart;
                    pattern.updateEvent(sel.lane, sel.index, ev);
                    any = true;
                }
            }
            if (! any) return true;

            processor.markDirty();
            repaint();
            notifySelectionChanged();

            processor.getUndoManager().beginNewTransaction("Nudge events");
            processor.getUndoManager().perform(
                new SetPatternAction(processor, this,
                                     std::move(before),
                                     pattern));
            return true;
        }

        return false;
    }
}

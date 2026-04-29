#pragma once

#include "EventOverridesDialog.h"
#include "Pattern/Pattern.h"
#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstddef>
#include <memory>

namespace B33p
{
    // Interactive sequencer canvas: four lanes stacked vertically, a
    // time ruler across the top, a narrow label column on the left,
    // and events drawn as filled rectangles that can be clicked to
    // create, dragged to move, edge-dragged to resize, and
    // delete-keyed to remove.
    //
    // Dragging is horizontal-only for MVP — the selected event stays
    // in its lane. Moving events between lanes is a post-MVP polish
    // item (delete-and-re-add is the workaround).
    //
    // Grid size is hardcoded at 100 ms for this commit; the next
    // Phase 4 commit introduces the grid-size / pattern-length
    // dropdowns and surfaces them here.
    class PatternGrid : public juce::Component
    {
    public:
        // Public so the inspector strip below the grid can read which
        // event the user has selected. lane = -1 means "no selection".
        struct Selection
        {
            int         lane  { -1 };
            std::size_t index {  0 };

            bool valid() const { return lane >= 0; }

            bool operator==(const Selection& o) const
            {
                return lane == o.lane && index == o.index;
            }
        };

        explicit PatternGrid(B33pProcessor& processor);

        // 0 (or negative) means "no snap" — clicks land at the exact
        // cursor position and grid lines are not drawn.
        void   setGridSeconds(double seconds);
        double getGridSeconds() const { return gridSeconds; }

        // Primary selection — the most-recently-clicked event.
        // Returns an invalid Selection if nothing is selected.
        // Inspector + single-target gestures use this; multi-target
        // gestures (delete, nudge, copy, drag) iterate getSelectedEvents.
        Selection getPrimarySelection() const;
        const std::vector<Selection>& getSelectedEvents() const { return selection; }

        void clearSelection();
        void selectAll();

        // Public clipboard / delete entry points so the Edit menu
        // can drive the same logic the keyboard shortcuts use.
        // pasteAtPlayhead pulls the playhead time from the processor
        // (the user parks it via the ruler click).
        void copySelectedToClipboard();
        void pasteFromClipboardAtPlayhead();
        void deleteSelected();

        // Edit-menu enable / disable inspection.
        bool hasSelection()    const { return ! selection.empty(); }
        bool hasClipboardData() const { return ! clipboard.empty(); }
        bool hasAnyEvents()    const;

        // Lane menu actions — also driven by the per-lane right-
        // click context menu. lane defaults to processor's current
        // selected lane when called from the menu bar.
        void generateRandomPatternInLane(int lane);
        void clearAllEventsInLane(int lane);

        // Pulls the per-lane name + mute state out of the pattern
        // and writes it into the label / button children. Called
        // after Open / New / undo so the UI tracks the model.
        void refreshLaneMetaFromPattern();

        // Fires whenever `selection` changes, including transitions to
        // / from "no selection". Used by the inspector strip to redraw.
        std::function<void()> onSelectionChanged;

        void paint(juce::Graphics& g) override;
        void resized()           override;

        void mouseDown       (const juce::MouseEvent& e) override;
        void mouseDrag       (const juce::MouseEvent& e) override;
        void mouseUp         (const juce::MouseEvent& e) override;
        void mouseMove       (const juce::MouseEvent& e) override;
        void mouseExit       (const juce::MouseEvent& e) override;
        void mouseDoubleClick(const juce::MouseEvent& e) override;

        bool keyPressed(const juce::KeyPress& key) override;

    private:
        // Selection writes go through these so onSelectionChanged
        // fires on every actual change without each call site
        // remembering to do so.
        void selectOnly       (const Selection& s);
        void addToSelection   (const Selection& s);
        void toggleInSelection(const Selection& s);
        bool isInSelection    (const Selection& s) const;
        void notifySelectionChanged();

        // One per selected event during a Move drag — captures the
        // event's original position so each can be shifted by the
        // same delta independently.
        struct DragSubject
        {
            Selection ref;
            Event     original;
        };

        enum class DragMode { None, Move, ResizeLeft, ResizeRight, DragVelocity, PendingCreate };

        struct HitResult
        {
            enum class Kind { None, Body, LeftEdge, RightEdge, TopEdge } kind { Kind::None };
            int         lane  { -1 };
            std::size_t index {  0 };
        };

        juce::Rectangle<float> plotArea() const;
        juce::Rectangle<float> laneArea(int lane) const;
        juce::Rectangle<float> eventRect(int lane, const Event& e) const;

        float  secondsToX(double seconds) const;
        double xToSeconds(float x) const;
        int    yToLane(float y) const;
        double snapSeconds(double seconds) const;

        HitResult hitTestEvent(juce::Point<float> p) const;

        B33pProcessor& processor;
        double         gridSeconds { 0.1 };

        // Multi-selection. The back element is the "primary" — the
        // most-recently-clicked event the inspector shows and that
        // resize gestures pin to.
        std::vector<Selection> selection;

        DragMode dragMode { DragMode::None };
        double   dragStartSeconds { 0.0 };
        Event    dragOriginalEvent;          // primary's original (Resize)
        std::vector<DragSubject> dragSubjects; // all (Move)
        int      pendingCreateLane { -1 };

        // mouseDown position kept around so PendingCreate can decide
        // "this was a click" vs "this was a drag" using the standard
        // 4 px JUCE drag threshold.
        juce::Point<float> mouseDownPos;

        // Whole-pattern snapshot taken on mouseDown so the gesture
        // (click + drag + release, or click + delete) commits as a
        // single UndoManager transaction in mouseUp.
        Pattern  gestureSnapshot;

        // Cursor hover tracking — drives the slight tint lift on the
        // event the user is about to click. lane = -1 = no hover.
        Selection hover;

        std::array<juce::Label,      Pattern::kNumLanes> laneNameLabels;
        std::array<juce::TextButton, Pattern::kNumLanes> muteButtons;
        std::array<juce::TextButton, Pattern::kNumLanes> soloButtons;

        // While a drag is active, the snap target time in seconds.
        // -1 means "no preview". Drawn as a vertical guide line so
        // the user sees where the event will land before releasing.
        double snapPreviewSeconds { -1.0 };

        // Set during mouseDrag when the requested position was
        // clamped against a pattern boundary — paints the snap
        // preview line red as a "you hit the wall" cue.
        bool   dragClampedAtWall  { false };

        // Internal app clipboard — what Cmd+C captured. Each entry
        // stores a lane and an Event whose startSeconds is RELATIVE
        // to the earliest copied event so paste can drop the group
        // at the playhead while preserving relative timing.
        struct ClipboardItem { int lane; Event event; };
        std::vector<ClipboardItem> clipboard;

        // Lazily-created popup window for editing per-event
        // overrides. Owned here so the window can survive PatternGrid
        // gestures and can be reset from the close callback.
        std::unique_ptr<EventOverridesDialogWindow> overridesWindow;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternGrid)
    };
}

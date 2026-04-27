#pragma once

#include "Pattern/Pattern.h"
#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstddef>

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
        };

        explicit PatternGrid(B33pProcessor& processor);

        // 0 (or negative) means "no snap" — clicks land at the exact
        // cursor position and grid lines are not drawn.
        void   setGridSeconds(double seconds);
        double getGridSeconds() const { return gridSeconds; }

        const Selection& getSelection() const { return selection; }
        void clearSelection();

        // Fires whenever `selection` changes, including transitions to
        // / from "no selection". Used by the inspector strip to redraw.
        std::function<void()> onSelectionChanged;

        void paint(juce::Graphics& g) override;

        void mouseDown       (const juce::MouseEvent& e) override;
        void mouseDrag       (const juce::MouseEvent& e) override;
        void mouseUp         (const juce::MouseEvent& e) override;
        void mouseMove       (const juce::MouseEvent& e) override;
        void mouseExit       (const juce::MouseEvent& e) override;
        void mouseDoubleClick(const juce::MouseEvent& e) override;

        bool keyPressed(const juce::KeyPress& key) override;

    private:
        // Bottleneck for selection writes so onSelectionChanged fires
        // on every transition without each call site remembering.
        void setSelection(const Selection& newSelection);

        enum class DragMode { None, Move, ResizeLeft, ResizeRight, PendingCreate };

        struct HitResult
        {
            enum class Kind { None, Body, LeftEdge, RightEdge } kind { Kind::None };
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

        Selection selection;

        DragMode dragMode { DragMode::None };
        double   dragStartSeconds { 0.0 };
        Event    dragOriginalEvent;
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

        // While a drag is active, the snap target time in seconds.
        // -1 means "no preview". Drawn as a vertical guide line so
        // the user sees where the event will land before releasing.
        double snapPreviewSeconds { -1.0 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternGrid)
    };
}
